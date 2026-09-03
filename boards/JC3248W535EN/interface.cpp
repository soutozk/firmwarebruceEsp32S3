#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <interface.h>

void _setup_gpio() {
    Serial.printf("[BOOT] ESP32-S3 iniciado: %s\n", DEVICE_NAME);
    Serial.println("[INPUT] touch disabled for Checkpoint 2B");

    // Match GhostESP: keep BL off until the LCD controller is initialized.
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, !TFT_BACKLIGHT_ON);
    bruceConfig.colorInverted = 0;
}

void _post_display_gpio() {
    pinMode(TFT_BL, OUTPUT);
    const bool pwmStarted = ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, (1U << TFT_BRIGHT_Bits) - 1U);
    Serial.printf(
        "[BACKLIGHT] %s: GPIO=%u ativo=%s PWM=%uHz/%ubit\n",
        pwmStarted ? "ativado" : "erro ao iniciar PWM",
        TFT_BL,
        TFT_BACKLIGHT_ON == HIGH ? "HIGH" : "LOW",
        TFT_BRIGHT_FREQ,
        TFT_BRIGHT_Bits
    );
}

void _setBrightness(uint8_t brightval) {
    const uint32_t maxDuty = (1U << TFT_BRIGHT_Bits) - 1U;
    ledcWrite(TFT_BL, (maxDuty * brightval) / 100U);
}

void InputHandler(void) {
    PrevPress = false;
    NextPress = false;
    UpPress = false;
    DownPress = false;
    SelPress = false;
    EscPress = false;
    AnyKeyPress = false;
    LongPress = false;
    touchPoint.pressed = false;
}

int getBattery() { return 0; }

bool isCharging() { return false; }

void powerOff() { esp_restart(); }

void checkReboot() {}
