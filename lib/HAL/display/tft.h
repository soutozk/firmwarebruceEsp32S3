#ifndef LIB_HAL_DISPLAY_TFT_H
#define LIB_HAL_DISPLAY_TFT_H
#include <pins_arduino.h>

#if !defined(USE_ARDUINO_GFX) && !defined(USE_LOVYANGFX) && !defined(USE_TFT_ESPI) && !defined(USE_M5GFX)
#define USE_TFT_ESPI
#endif

class TFT_eSPI;
class tft_sprite;
class tft_logger;
#include <SPI.h>
#if defined(USE_TFT_ESPI)
#include "tftespi.h"

#elif defined(USE_ARDUINO_GFX)
#if defined(BOARD_JC3248W535EN)
#include "jc3248w535en_display.h"
#else
#include "ardgfx.h"
#endif

#elif defined(USE_LOVYANGFX)
#include "lovyan.h"

#elif defined(USE_M5GFX)
#include "m5gfx.h"

#endif
#endif // LIB_HAL_DISPLAY_TFT_H
