#pragma once

// Standalone physical checkpoint.  This path intentionally bypasses the
// Bruce/Arduino_GFX renderer so the QSPI + DMA pipeline can be validated in
// the same shape as the known-good OBD implementation.
void jc3248w535en_display_init();
[[noreturn]] void jc3248w535en_display_test();
