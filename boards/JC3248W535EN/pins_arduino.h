#ifndef JC3248W535EN_PINS_ARDUINO_H
#define JC3248W535EN_PINS_ARDUINO_H

#include <stdint.h>

#define DEVICE_NAME "JC3248W535EN"
#define BOARD_JC3248W535EN 1

// Native panel coordinates are 320 x 480; Bruce uses rotation 1 for landscape.
#define TFT_WIDTH 320
#define TFT_HEIGHT 480
#define TFT_BL 1
#define TFT_BRIGHT_FREQ 5000
#define TFT_BRIGHT_Bits 10
#define TFT_BACKLIGHT_ON HIGH

#define HAS_SCREEN 1
// Touch is intentionally deferred to Checkpoint 3. Keep its pins below, but
// do not expose HAS_TOUCH or initialize its I2C controller in Checkpoint 2B.
#define JC3248W535EN_TOUCH_DISABLED 1
#define HAS_BTN 0
#define BTN_ALIAS "Touch"
#define ROTATION 1
#define MINBRIGHT 1

// Arduino_GFX AXS15231B QSPI panel.
#define USE_ARDUINO_GFX 1
#define TFT_DATABUS_N 1
#define TFT_DISPLAY_DRIVER_N 22
#define ESP32QSPI_FREQUENCY 40000000
#define ESP32QSPI_SPI_MODE SPI_MODE3
// Exact JC3248W535EN vendor table from the working OBD/reference firmware.
// The board-specific wrapper adds the software reset/preamble and QSPI draw
// semantics below.
#define TFT_INIT_OPERATIONS jc3248w535en_init_operations
#define TFT_INIT_OPERATIONS_LEN sizeof(jc3248w535en_init_operations)
#define TFT_DATABUS JC3248W535ENQSPI
#define TFT_DISPLAY_DRIVER JC3248W535ENAXS15231B
#define TFT_CS 45
#define TFT_SCLK 47
#define TFT_D0 21
#define TFT_D1 48
#define TFT_D2 40
#define TFT_D3 39
#define TFT_TE 38
#define TFT_RST -1
#define TFT_ROTATION 0
#define TFT_IPS 0
#define TFT_COL_OFS1 0
#define TFT_ROW_OFS1 0
#define TFT_COL_OFS2 0
#define TFT_ROW_OFS2 0

// AXS15231B capacitive touch controller.
#define AXS15231B_I2C_ADDR 0x3B
#define AXS15231B_TOUCH_SDA 4
#define AXS15231B_TOUCH_SCL 8
#define SDA AXS15231B_TOUCH_SDA
#define SCL AXS15231B_TOUCH_SCL

// No SD socket is present on this board.
#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

// Conservative defaults for external one-wire modules.
#define GROVE_SDA 6
#define GROVE_SCL 7
#define BAD_TX GROVE_SDA
#define BAD_RX GROVE_SCL
#define SERIAL_TX 6
#define SERIAL_RX 7
#define GPS_SERIAL_TX 6
#define GPS_SERIAL_RX 7
#define TXLED 6
#define RXLED 7
#define LED_ON HIGH
#define LED_OFF LOW

#define CC1101_GDO0_PIN 3
#define CC1101_SS_PIN 10
#define CC1101_MOSI_PIN 11
#define CC1101_SCK_PIN 12
#define CC1101_MISO_PIN 13
#define USE_CC1101_VIA_SPI

#define SPI_SCK_PIN 12
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SS_PIN 10
#define SCK SPI_SCK_PIN
#define MISO SPI_MISO_PIN
#define MOSI SPI_MOSI_PIN
#define SS SPI_SS_PIN

#define FP 1
#define FM 2
#define FG 3
#define SMOOTH_FONT 1

#endif
