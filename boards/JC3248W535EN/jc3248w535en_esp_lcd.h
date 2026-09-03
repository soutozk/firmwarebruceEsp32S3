#pragma once

#include <stdint.h>

void jc3248w535en_esp_lcd_init();
[[noreturn]] void jc3248w535en_esp_lcd_test();
// Presents an already rotated physical 320x480 RGB565 framebuffer through the
// validated panel path. This function performs no geometry transformation.
void jc3248w535en_esp_lcd_present_physical(const uint16_t *physical_frame);
