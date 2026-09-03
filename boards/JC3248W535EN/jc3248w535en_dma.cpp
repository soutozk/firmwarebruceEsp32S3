#include "jc3248w535en_dma.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "../../lib/HAL/display/jc3248w535en.h"
#include "pins_arduino.h"

namespace {
constexpr uint32_t kWidth = 320;
constexpr uint32_t kHeight = 480;
constexpr uint32_t kPixels = kWidth * kHeight;
constexpr uint32_t kTransportPixels = 15360;
constexpr uint32_t kTransportBytes = kTransportPixels * sizeof(uint16_t);
// Logical RGB565 values. panel565() converts them to the byte order expected
// by the AXS15231B QSPI path (equivalent to LV_COLOR_16_SWAP=1).
constexpr uint16_t kRed = 0xF800;
constexpr uint16_t kGreen = 0x07E0;
constexpr uint16_t kBlue = 0x001F;
constexpr uint16_t kWhite = 0xFFFF;
constexpr uint16_t kBlack = 0x0000;

spi_device_handle_t s_lcd = nullptr;
SemaphoreHandle_t s_tx_done = nullptr;
SemaphoreHandle_t s_te = nullptr;
uint8_t *s_command_dma = nullptr;
uint16_t *s_transport[2] = {nullptr, nullptr};
spi_transaction_ext_t s_color_tx[2] = {};

constexpr uint32_t qspi_command(uint8_t opcode, uint8_t command) {
    // AXS15231B QSPI framing: (opcode << 24) | (command << 8).
    return (static_cast<uint32_t>(opcode) << 24) |
           (static_cast<uint32_t>(command) << 8);
}

void IRAM_ATTR color_trans_done(spi_transaction_t *) {
    BaseType_t high_task_woken = pdFALSE;
    if (s_tx_done) xSemaphoreGiveFromISR(s_tx_done, &high_task_woken);
    if (high_task_woken) portYIELD_FROM_ISR();
}

void IRAM_ATTR te_falling_edge(void *) {
    BaseType_t high_task_woken = pdFALSE;
    if (s_te) xSemaphoreGiveFromISR(s_te, &high_task_woken);
    if (high_task_woken) portYIELD_FROM_ISR();
}

bool tx_param(uint8_t command, const uint8_t *data, size_t length) {
    spi_transaction_ext_t tx = {};
    tx.base.cmd = qspi_command(0x02, command);
    if (length) {
        if (length > 64 || !s_command_dma) return false;
        memcpy(s_command_dma, data, length);
        tx.base.tx_buffer = s_command_dma;
        tx.base.length = length * 8;
    }
    return spi_device_polling_transmit(s_lcd, &tx.base) == ESP_OK;
}

bool apply_init_table() {
    const uint8_t *ops = jc3248w535en_init_operations;
    const size_t length = sizeof(jc3248w535en_init_operations);
    for (size_t i = 0; i < length; ++i) {
        switch (ops[i]) {
            case BEGIN_WRITE:
            case END_WRITE:
                break;
            case WRITE_COMMAND_8:
                if (++i >= length || !tx_param(ops[i], nullptr, 0)) return false;
                break;
            case WRITE_C8_BYTES: {
                if (i + 2 >= length) return false;
                const uint8_t command = ops[++i];
                const uint8_t count = ops[++i];
                if (i + count >= length || !tx_param(command, ops + i + 1, count)) return false;
                i += count;
                break;
            }
            case DELAY:
                if (++i >= length) return false;
                delay(ops[i]);
                break;
            default:
                Serial.printf("[JC3248] unsupported init op=0x%02X at %u\n", ops[i], (unsigned)i);
                return false;
        }
    }
    return true;
}

void wait_tx_done() {
    xSemaphoreTake(s_tx_done, portMAX_DELAY);
    spi_transaction_t *completed = nullptr;
    spi_device_get_trans_result(s_lcd, &completed, portMAX_DELAY);
}

void wait_for_te() {
    // Match the OBD falling-edge TE synchronization, but keep a timeout so a
    // disconnected TE line cannot deadlock this physical checkpoint.
    if (xSemaphoreTake(s_te, pdMS_TO_TICKS(50)) != pdTRUE) {
        Serial.println("[JC3248] TE timeout; transmitting frame");
    }
}

uint16_t panel565(uint16_t color) {
    // Equivalent to LV_COLOR_16_SWAP=1: DMA memory contains high byte first.
    return static_cast<uint16_t>((color >> 8) | (color << 8));
}

void prepare_color_chunk(uint32_t chunk, const uint16_t *frame, uint32_t offset, uint32_t count) {
    memcpy(s_transport[chunk], frame + offset, count * sizeof(uint16_t));
    const uint8_t command = (offset == 0) ? 0x2C : 0x3C;
    s_color_tx[chunk] = {};
    s_color_tx[chunk].base.flags = SPI_TRANS_MODE_QIO;
    s_color_tx[chunk].base.cmd = qspi_command(0x32, command);
    s_color_tx[chunk].base.tx_buffer = s_transport[chunk];
    s_color_tx[chunk].base.length = count * 16;
    Serial.printf("[CHUNK %u] prepare\n", (unsigned)(offset / kTransportPixels));
    Serial.printf("[CHUNK %u] %s\n", (unsigned)(offset / kTransportPixels), command == 0x2C ? "RAMWR" : "RAMWRC");
}

void send_frame(const char *name, const uint16_t *frame) {
    wait_for_te();
    Serial.printf("[FRAME %s] start\n", name);
    const uint32_t chunks = (kPixels + kTransportPixels - 1) / kTransportPixels;

    uint32_t slot = 0;
    uint32_t offset = 0;
    uint32_t count = (kPixels < kTransportPixels) ? kPixels : kTransportPixels;
    prepare_color_chunk(slot, frame, offset, count);
    Serial.println("[CHUNK 0] TX start");
    ESP_ERROR_CHECK(spi_device_queue_trans(s_lcd, &s_color_tx[slot].base, portMAX_DELAY));

    for (uint32_t index = 1; index < chunks; ++index) {
        offset = index * kTransportPixels;
        count = ((kPixels - offset) < kTransportPixels) ? (kPixels - offset) : kTransportPixels;
        slot ^= 1U;
        // The alternate DMA buffer is prepared while the previous transfer is
        // active.  The previous buffer is not touched until TX done arrives.
        prepare_color_chunk(slot, frame, offset, count);
        wait_tx_done();
        Serial.printf("[CHUNK %u] TX start\n", (unsigned)index);
        ESP_ERROR_CHECK(spi_device_queue_trans(s_lcd, &s_color_tx[slot].base, portMAX_DELAY));
    }
    wait_tx_done();
    Serial.printf("[FRAME %s] done\n", name);
}

void fill_solid(uint16_t *frame, uint16_t color) {
    for (uint32_t i = 0; i < kPixels; ++i) frame[i] = panel565(color);
}

void fill_quadrants(uint16_t *frame) {
    const uint16_t colors[2][2] = {
        {panel565(kRed), panel565(kGreen)},
        {panel565(kBlue), panel565(kWhite)},
    };
    for (uint32_t y = 0; y < kHeight; ++y) {
        for (uint32_t x = 0; x < kWidth; ++x) {
            const bool right = x >= kWidth / 2;
            const bool bottom = y >= kHeight / 2;
            const bool vertical_border = (x == kWidth / 2 - 1) || (x == kWidth / 2);
            const bool horizontal_border = (y == kHeight / 2 - 1) || (y == kHeight / 2);
            frame[y * kWidth + x] = (vertical_border || horizontal_border)
                ? panel565(kBlack)
                : colors[bottom][right];
        }
    }
}
} // namespace

void jc3248w535en_display_init() {
    spi_bus_config_t bus = {};
    bus.mosi_io_num = TFT_D0;
    bus.miso_io_num = TFT_D1;
    bus.sclk_io_num = TFT_SCLK;
    bus.quadwp_io_num = TFT_D2;
    bus.quadhd_io_num = TFT_D3;
    bus.max_transfer_sz = kTransportBytes;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t device = {};
    device.command_bits = 32;
    device.address_bits = 0;
    device.mode = SPI_MODE3;
    device.clock_speed_hz = 40 * 1000 * 1000;
    device.spics_io_num = TFT_CS;
    device.flags = SPI_DEVICE_HALFDUPLEX;
    device.queue_size = 2;
    device.post_cb = color_trans_done;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &device, &s_lcd));
    ESP_ERROR_CHECK(spi_device_acquire_bus(s_lcd, portMAX_DELAY));

    s_tx_done = xSemaphoreCreateCounting(2, 0);
    s_te = xSemaphoreCreateBinary();
    s_command_dma = static_cast<uint8_t *>(heap_caps_malloc(64, MALLOC_CAP_DMA));
    s_transport[0] = static_cast<uint16_t *>(heap_caps_malloc(kTransportBytes, MALLOC_CAP_DMA));
    s_transport[1] = static_cast<uint16_t *>(heap_caps_malloc(kTransportBytes, MALLOC_CAP_DMA));
    if (!s_tx_done || !s_te || !s_command_dma || !s_transport[0] || !s_transport[1]) {
        Serial.println("[JC3248] DMA allocation failed");
        abort();
    }

    gpio_config_t te_cfg = {};
    te_cfg.intr_type = GPIO_INTR_NEGEDGE;
    te_cfg.mode = GPIO_MODE_INPUT;
    te_cfg.pin_bit_mask = 1ULL << TFT_TE;
    te_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&te_cfg));
    esp_err_t isr_status = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (isr_status != ESP_OK && isr_status != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(isr_status);
    ESP_ERROR_CHECK(gpio_isr_handler_add(static_cast<gpio_num_t>(TFT_TE), te_falling_edge, nullptr));

    Serial.println("[JC3248] SPI2 QSPI initialized");
    if (!tx_param(0x01, nullptr, 0)) abort();
    delay(120);
    if (!tx_param(0x11, nullptr, 0)) abort();
    delay(100);
    const uint8_t madctl = 0x00;
    const uint8_t colmod = 0x55;
    if (!tx_param(0x36, &madctl, 1) || !tx_param(0x3A, &colmod, 1) || !apply_init_table()) abort();
    Serial.println("[JC3248] AXS15231B initialized");
    Serial.println("[JC3248] Custom init applied");
    if (!tx_param(0x20, nullptr, 0) || !tx_param(0x29, nullptr, 0)) abort();
    Serial.println("[JC3248] Display ON");
    Serial.println("[JC3248] Panel IO initialized");
}

[[noreturn]] void jc3248w535en_display_test() {
    uint16_t *frame = static_cast<uint16_t *>(heap_caps_malloc(kPixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
    if (!frame) {
        Serial.println("[JC3248] framebuffer PSRAM allocation failed");
        abort();
    }
    Serial.println("[JC3248] Starting hardware display test");
    Serial.println("[JC3248] Portrait 320x480, RGB565, DMA double buffer");

    fill_solid(frame, kRed);
    send_frame("RED", frame);
    delay(2000);
    fill_solid(frame, kGreen);
    send_frame("GREEN", frame);
    delay(2000);
    fill_solid(frame, kBlue);
    send_frame("BLUE", frame);
    delay(2000);
    fill_quadrants(frame);
    send_frame("QUADRANTS", frame);
    Serial.println("[JC3248] quadrant pattern held; no further frame");
    Serial.flush();
    while (true) vTaskDelay(pdMS_TO_TICKS(1000));
}
