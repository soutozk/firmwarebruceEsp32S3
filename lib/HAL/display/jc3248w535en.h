#pragma once

// Arduino_GFX's stock AXS15231B class is close to the panel protocol, but its
// address window emits RASET and its pixel path always uses RAMWRC.  The
// functional OBD firmware uses QSPI CASET-only windows and selects RAMWR for
// the first row.  Keep the Arduino_GFX integration while overriding only
// those hardware-specific operations.
#define private public
#include <databus/Arduino_ESP32QSPI.h>
#include <display/Arduino_AXS15231B.h>
#undef private

// Exact JC3248W535EN AXS15231B vendor sequence from the working reference firmware.
static const uint8_t jc3248w535en_init_operations[] = {
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xBB, 8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0xA5,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA0, 17,
    0xC0, 0x10, 0x00, 0x02, 0x00, 0x00, 0x04, 0x3F,
    0x20, 0x05, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA2, 31,
    0x30, 0x3C, 0x24, 0x14, 0xD0, 0x20, 0xFF, 0xE0,
    0x40, 0x19, 0x80, 0x80, 0x80, 0x20, 0xf9, 0x10,
    0x02, 0xff, 0xff, 0xF0, 0x90, 0x01, 0x32, 0xA0,
    0x91, 0xE0, 0x20, 0x7F, 0xFF, 0x00, 0x5A,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD0, 30,
    0xE0, 0x40, 0x51, 0x24, 0x08, 0x05, 0x10, 0x01,
    0x20, 0x15, 0x42, 0xC2, 0x22, 0x22, 0xAA, 0x03,
    0x10, 0x12, 0x60, 0x14, 0x1E, 0x51, 0x15, 0x00,
    0x8A, 0x20, 0x00, 0x03, 0x3A, 0x12,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA3, 22,
    0xA0, 0x06, 0xAa, 0x00, 0x08, 0x02, 0x0A, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x55, 0x55,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC1, 30,
    0x31, 0x04, 0x02, 0x02, 0x71, 0x05, 0x24, 0x55,
    0x02, 0x00, 0x41, 0x00, 0x53, 0xFF, 0xFF, 0xFF,
    0x4F, 0x52, 0x00, 0x4F, 0x52, 0x00, 0x45, 0x3B,
    0x0B, 0x02, 0x0d, 0x00, 0xFF, 0x40,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC3, 11,
    0x00, 0x00, 0x00, 0x50, 0x03, 0x00, 0x00, 0x00,
    0x01, 0x80, 0x01,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC4, 29,
    0x00, 0x24, 0x33, 0x80, 0x00, 0xea, 0x64, 0x32,
    0xC8, 0x64, 0xC8, 0x32, 0x90, 0x90, 0x11, 0x06,
    0xDC, 0xFA, 0x00, 0x00, 0x80, 0xFE, 0x10, 0x10,
    0x00, 0x0A, 0x0A, 0x44, 0x50,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC5, 23,
    0x18, 0x00, 0x00, 0x03, 0xFE, 0x3A, 0x4A, 0x20,
    0x30, 0x10, 0x88, 0xDE, 0x0D, 0x08, 0x0F, 0x0F,
    0x01, 0x3A, 0x4A, 0x20, 0x10, 0x10, 0x00,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC6, 20,
    0x05, 0x0A, 0x05, 0x0A, 0x00, 0xE0, 0x2E, 0x0B,
    0x12, 0x22, 0x12, 0x22, 0x01, 0x03, 0x00, 0x3F,
    0x6A, 0x18, 0xC8, 0x22,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC7, 20,
    0x50, 0x32, 0x28, 0x00, 0xa2, 0x80, 0x8f, 0x00,
    0x80, 0xff, 0x07, 0x11, 0x9c, 0x67, 0xff, 0x24,
    0x0c, 0x0d, 0x0e, 0x0f,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xC9, 4,
    0x33, 0x44, 0x44, 0x01,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xCF, 27,
    0x2C, 0x1E, 0x88, 0x58, 0x13, 0x18, 0x56, 0x18,
    0x1E, 0x68, 0x88, 0x00, 0x65, 0x09, 0x22, 0xC4,
    0x0C, 0x77, 0x22, 0x44, 0xAA, 0x55, 0x08, 0x08,
    0x12, 0xA0, 0x08,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD5, 30,
    0x40, 0x8E, 0x8D, 0x01, 0x35, 0x04, 0x92, 0x74,
    0x04, 0x92, 0x74, 0x04, 0x08, 0x6A, 0x04, 0x46,
    0x03, 0x03, 0x03, 0x03, 0x82, 0x01, 0x03, 0x00,
    0xE0, 0x51, 0xA1, 0x00, 0x00, 0x00,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD6, 30,
    0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE,
    0x93, 0x00, 0x01, 0x83, 0x07, 0x07, 0x00, 0x07,
    0x07, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x00, 0x84, 0x00, 0x20, 0x01, 0x00,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD7, 19,
    0x03, 0x01, 0x0b, 0x09, 0x0f, 0x0d, 0x1E, 0x1F,
    0x18, 0x1d, 0x1f, 0x19, 0x40, 0x8E, 0x04, 0x00,
    0x20, 0xA0, 0x1F,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD8, 12,
    0x02, 0x00, 0x0a, 0x08, 0x0e, 0x0c, 0x1E, 0x1F,
    0x18, 0x1d, 0x1f, 0x19,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xD9, 12,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xDD, 12,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xDF, 8,
    0x44, 0x73, 0x4B, 0x69, 0x00, 0x0A, 0x02, 0x90,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE0, 17,
    0x3B, 0x28, 0x10, 0x16, 0x0c, 0x06, 0x11, 0x28,
    0x5c, 0x21, 0x0D, 0x35, 0x13, 0x2C, 0x33, 0x28,
    0x0D,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE1, 17,
    0x37, 0x28, 0x10, 0x16, 0x0b, 0x06, 0x11, 0x28,
    0x5C, 0x21, 0x0D, 0x35, 0x14, 0x2C, 0x33, 0x28,
    0x0F,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE2, 17,
    0x3B, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x35,
    0x44, 0x32, 0x0C, 0x14, 0x14, 0x36, 0x3A, 0x2F,
    0x0D,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE3, 17,
    0x37, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x35,
    0x44, 0x32, 0x0C, 0x14, 0x14, 0x36, 0x32, 0x2F,
    0x0F,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE4, 17,
    0x3B, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x39,
    0x44, 0x2E, 0x0C, 0x14, 0x14, 0x36, 0x3A, 0x2F,
    0x0D,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xE5, 17,
    0x37, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x39,
    0x44, 0x2E, 0x0C, 0x14, 0x14, 0x36, 0x3A, 0x2F,
    0x0F,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA4, 16,
    0x85, 0x85, 0x95, 0x82, 0xAF, 0xAA, 0xAA, 0x80,
    0x10, 0x30, 0x40, 0x40, 0x20, 0xFF, 0x60, 0x30,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xA4, 4,
    0x85, 0x85, 0x95, 0x85,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xBB, 8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    END_WRITE,
    BEGIN_WRITE,
    // Command-only writes must use WRITE_COMMAND_8.  A zero-length
    // WRITE_C8_BYTES leaves a non-null tx_buffer with no MOSI phase, which
    // ESP-IDF rejects during spi_device_polling_start().
    WRITE_COMMAND_8, 0x13,
    END_WRITE,
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x11,
    END_WRITE,
    DELAY, 120,
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0x2C, 4,
    0x00, 0x00, 0x00, 0x00,
    END_WRITE
};

// These helpers are declared always_inline by Arduino_GFX but their stock
// definitions live in a separate translation unit.  Keep the definitions
// visible here so the custom pixel path remains link/LTO-safe.
GFX_INLINE void Arduino_ESP32QSPI::CS_HIGH(void) { *_csPortSet = _csPinMask; }
GFX_INLINE void Arduino_ESP32QSPI::CS_LOW(void) { *_csPortClr = _csPinMask; }
GFX_INLINE void Arduino_ESP32QSPI::POLL_START() { spi_device_polling_start(_handle, _spi_tran, portMAX_DELAY); }
GFX_INLINE void Arduino_ESP32QSPI::POLL_END() { spi_device_polling_end(_handle, portMAX_DELAY); }

class JC3248W535ENQSPI : public Arduino_ESP32QSPI {
public:
    using Arduino_ESP32QSPI::Arduino_ESP32QSPI;

    bool begin(int32_t speed = GFX_NOT_DEFINED, int8_t dataMode = GFX_NOT_DEFINED) override {
        const bool ok = Arduino_ESP32QSPI::begin(40000000, SPI_MODE3);
        if (ok) Serial.println("[JC3248] SPI2 QSPI initialized");
        return ok;
    }

    void setPixelCommand(uint8_t command) { _pixel_command = command; }

    void writePixels(uint16_t *data, uint32_t len) override {
        writePixelBytes(data, len, true);
    }

    void writeRepeat(uint16_t color, uint32_t len) override {
        // _buffer16 and _buffer32 are two views of the same DMA allocation.
        // Do not fill a uint16 scratch buffer and then repack it in place:
        // that overwrites pixels which have not been read yet and produces
        // the characteristic striped/column-corrupted screen.  This mirrors
        // Arduino_ESP32QSPI::writeRepeat and writes the packed DMA words once.
        const uint32_t n = (len > ESP32QSPI_MAX_PIXELS_AT_ONCE) ? ESP32QSPI_MAX_PIXELS_AT_ONCE : len;
        uint32_t packed;
        MSB_32_16_16_SET(packed, color, color);
        for (uint32_t i = 0; i < (n + 1U) / 2U; ++i) _buffer32[i] = packed;

        bool first = true;
        while (len) {
            const uint32_t count = (len < n) ? len : n;
            CS_LOW();
            if (first) {
                _spi_tran_ext.base.flags = SPI_TRANS_MODE_QIO;
                _spi_tran_ext.base.cmd = 0x32;
                _spi_tran_ext.base.addr = ((uint32_t)_pixel_command) << 8;
                first = false;
            } else {
                _spi_tran_ext.base.flags = SPI_TRANS_MODE_QIO;
                _spi_tran_ext.base.cmd = 0x32;
                _spi_tran_ext.base.addr = ((uint32_t)0x3C) << 8;
            }
            _spi_tran_ext.base.tx_buffer = _buffer16;
            _spi_tran_ext.base.length = count << 4;
            POLL_START();
            POLL_END();
            CS_HIGH();
            len -= count;
        }
        _pixel_command = 0x3C;
    }

    private:
    void writePixelBytes(uint16_t *data, uint32_t len, bool first) {
        while (len) {
            const uint32_t count = (len > ESP32QSPI_MAX_PIXELS_AT_ONCE) ? ESP32QSPI_MAX_PIXELS_AT_ONCE : len;
            const uint32_t pairs = count >> 1;
            for (uint32_t i = 0; i < pairs; ++i) {
                MSB_32_16_16_SET(_buffer32[i], data[0], data[1]);
                data += 2;
            }
            if (count & 1U) {
                const uint16_t last = *data++;
                MSB_16_SET(_buffer16[count - 1], last);
            }

            CS_LOW();
            if (first) {
                _spi_tran_ext.base.flags = SPI_TRANS_MODE_QIO;
                _spi_tran_ext.base.cmd = 0x32;
                _spi_tran_ext.base.addr = ((uint32_t)_pixel_command) << 8;
                first = false;
            } else {
                _spi_tran_ext.base.flags = SPI_TRANS_MODE_QIO;
                _spi_tran_ext.base.cmd = 0x32;
                _spi_tran_ext.base.addr = ((uint32_t)0x3C) << 8;
            }
            _spi_tran_ext.base.tx_buffer = _buffer32;
            _spi_tran_ext.base.length = count << 4;
            POLL_START();
            POLL_END();
            CS_HIGH();
            len -= count;
        }
        _pixel_command = 0x3C;
    }

    uint8_t _pixel_command = 0x3C;
};

class JC3248W535ENAXS15231B : public Arduino_AXS15231B {
public:
    using Arduino_AXS15231B::Arduino_AXS15231B;

    bool begin(int32_t speed = GFX_NOT_DEFINED) override {
        const bool ok = Arduino_TFT::begin(40000000);
        if (ok) Serial.println("[JC3248] Panel IO initialized");
        return ok;
    }

    void setRotation(uint8_t rotation) override {
        // Keep Arduino_GFX's logical dimensions and also program the panel's
        // MADCTL orientation.  Without this, rotation 1 exposes a 480-pixel
        // logical X range to the native 320x480 controller, so every QSPI
        // frame is addressed outside the panel's column range.
        Arduino_AXS15231B::setRotation(rotation);
    }

protected:
    void tftInit() override {
        // RST is physically NC on this board: use the panel's software reset.
        _bus->writeCommand(AXS15231B_SWRESET);
        delay(120);
        _bus->writeC8D8(AXS15231B_SLPOUT, 0x00);
        delay(100);
        _bus->writeC8D8(AXS15231B_MADCTL, 0x00);
        _bus->writeC8D8(AXS15231B_COLMOD, 0x55);
        Serial.println("[JC3248] AXS15231B initialized");
        _bus->batchOperation(_init_operations, _init_operations_len);
        Serial.println("[JC3248] Custom init applied");
        invertDisplay(false);
        _bus->writeCommand(AXS15231B_DISPON);
        Serial.println("[JC3248] Display ON");
    }

    void writeAddrWindow(int16_t x, int16_t y, uint16_t w, uint16_t h) override {
        // The reference panel driver emits CASET for every draw operation.
        // Arduino_GFX caches the last window, but this panel has no RASET in
        // QSPI mode and RAMWRC continues from the controller's current row;
        // re-issuing CASET keeps each frame aligned instead of inheriting a
        // stale/partial window from the previous transfer.
        _currentX = x;
        _currentW = w;
        x += _xStart;
        _bus->writeC8D16D16(AXS15231B_CASET, x, x + w - 1);
        // QSPI functional path deliberately does not send RASET.
        auto *qspi = static_cast<JC3248W535ENQSPI *>(_bus);
        qspi->setPixelCommand((y == 0) ? AXS15231B_RAMWR : 0x3C);
    }
};
