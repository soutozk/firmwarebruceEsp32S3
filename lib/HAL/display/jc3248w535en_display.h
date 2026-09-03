#ifndef LIB_HAL_JC3248W535EN_DISPLAY_H
#define LIB_HAL_JC3248W535EN_DISPLAY_H

#include "tft_defines.h"
#include <Arduino.h>
#include <SPI.h>
#include <vector>

class Arduino_GFX;
class tft_sprite;

// Bruce-facing display adapter.  It deliberately has the same surface API as
// the normal tft_display class, but all drawing happens in one PSRAM-backed
// logical framebuffer (480x320).  The esp_lcd backend is touched only by
// present().
class tft_display {
protected:
    uint16_t *_framebuffer = nullptr;
    uint16_t *_physicalFramebuffer = nullptr;
    int16_t _width = 480;
    int16_t _height = 320;

public:
    explicit tft_display(int16_t _W = 480, int16_t _H = 320);
    friend class tft_sprite;

    void begin(uint32_t speed = 0);
    void init(uint8_t tc = 0);
    void setRotation(uint8_t r);
    void drawPixel(int32_t x, int32_t y, uint32_t color);
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color);
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color);
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
    void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
    void fillScreen(uint32_t color);
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color);
    void fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color);
    void drawArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle,
                 uint32_t fg_color, uint32_t bg_color, bool smoothArc = true);
    void drawWideLine(float ax, float ay, float bx, float by, float wd, uint32_t fg_color,
                      uint32_t bg_color = 0x00FFFFFF);
    void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
    void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8, uint16_t *cmap);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8, uint16_t *cmap);
    void invertDisplay(bool i);
    void sleep(bool value);
    void setSleepMode(bool mode) { (void)mode; }
    void setSwapBytes(bool swap);
    bool getSwapBytes() const;
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) const;

    int16_t textWidth(const String &s, uint8_t font = 1) const;
    int16_t textWidth(const char *s, uint8_t font = 1) const;
    void setCursor(int16_t x, int16_t y);
    int16_t getCursorX() const;
    int16_t getCursorY() const;
    void setTextSize(uint8_t s);
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t b, bool bgfill = false);
    void setTextDatum(uint8_t d);
    uint8_t getTextDatum() const;
    void setTextFont(uint8_t f);
    void setTextWrap(bool wrapX, bool wrapY = false);
    int16_t drawString(const String &string, int32_t x, int32_t y, uint8_t font = 1);
    int16_t drawCentreString(const String &string, int32_t x, int32_t y, uint8_t font = 1);
    int16_t drawRightString(const String &string, int32_t x, int32_t y, uint8_t font = 1);
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    template <typename T> size_t print(const T &val) { return print(String(val)); }
    template <typename T> size_t println(const T &val) { return print(String(val) + "\n"); }
    size_t print(const String &s);
    size_t println();
    size_t printf(const char *fmt, ...);

    int16_t width() const;
    int16_t height() const;
    SPIClass &getSPIinstance() const;
    void writecommand(uint8_t c);
    uint32_t getTextColor() const;
    uint32_t getTextBgColor() const;
    uint8_t getTextSize() const;
    uint8_t getRotation() const;
    int16_t fontHeight(int16_t font = 1) const;
    Arduino_GFX *native();

    // Explicit frame presentation boundary used by checkpoint 2A and later
    // by the Bruce UI integration.
    void present();

private:
    void ensureFramebuffer();
    void setPixel(int32_t x, int32_t y, uint16_t color);
    void drawGlyph(int32_t x, int32_t y, uint8_t c);
    int16_t drawAlignedString(const String &s, int32_t x, int32_t y, uint8_t datum);
    bool _swapBytes = false;
    uint32_t _textColor = TFT_WHITE;
    uint32_t _textBgColor = TFT_BLACK;
    uint8_t _textSize = 1;
    uint8_t _textDatum = TL_DATUM;
    uint8_t _textFont = 1;
    uint8_t _rotation = 1;
    int16_t _cursorX = 0;
    int16_t _cursorY = 0;
    bool _wrap = true;
    bool _dirty = true;
    uint32_t _lastPresentMs = 0;
};

// Lightweight sprite compatibility.  It is kept in RAM/PSRAM and only
// copies into the parent framebuffer when pushSprite() is called.
class tft_sprite {
public:
    explicit tft_sprite(tft_display *parent);
    ~tft_sprite() = default;
    void *createSprite(int16_t w, int16_t h, uint8_t frames = 1);
    void deleteSprite();
    void fillScreen(uint32_t color);
    void setColorDepth(uint8_t depth);
    void setCursor(int16_t x, int16_t y);
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t b);
    void setTextSize(uint8_t s);
    void setTextDatum(uint8_t d);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color);
    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color);
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void pushSprite(int32_t x, int32_t y, uint32_t transparent = TFT_TRANSPARENT);
    void pushToSprite(tft_sprite *dest, int32_t x, int32_t y, uint32_t transparent = TFT_TRANSPARENT);
    int16_t width() const;
    int16_t height() const;
    int16_t fontHeight(int16_t font = 1) const;
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
    void drawPixel(int32_t x, int32_t y, uint32_t color);
    void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg = 0);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8, uint16_t *cmap);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8, uint16_t *cmap);
    void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
    void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
    int16_t drawString(const String &string, int32_t x, int32_t y, uint8_t font = 1);

private:
    bool _hasBuffer() const;
    void setPixel(int32_t x, int32_t y, uint16_t color);
    tft_display *_display = nullptr;
    int16_t _width = 0, _height = 0;
    std::vector<uint16_t> _buffer;
    uint8_t _colorDepth = 16;
    int16_t _cursorX = 0, _cursorY = 0;
    uint32_t _textColor = TFT_WHITE, _textBgColor = TFT_BLACK;
    uint8_t _textSize = 1, _textDatum = TL_DATUM;
};

#endif
