#include "../../lib/HAL/display/jc3248w535en_display.h"

#include "jc3248w535en_esp_lcd.h"
#include <esp_heap_caps.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

// Reuse Bruce's existing Adafruit-compatible 5x7 glyph table.  It is only
// read while drawing into RAM; it never talks to the LCD.
#include "../../lib/TFT_eSPI/Fonts/glcdfont.c"

namespace {
constexpr uint32_t kLogicalWidth = 480;
constexpr uint32_t kLogicalHeight = 320;
constexpr uint32_t kPhysicalWidth = 320;
constexpr uint32_t kPhysicalHeight = 480;
constexpr uint32_t kLogicalPixels = kLogicalWidth * kLogicalHeight;
constexpr uint32_t kPhysicalPixels = kPhysicalWidth * kPhysicalHeight;
constexpr size_t kLogicalFramebufferBytes = kLogicalPixels * sizeof(uint16_t);
constexpr size_t kPhysicalFramebufferBytes = kPhysicalPixels * sizeof(uint16_t);
constexpr uint32_t kMinimumPresentIntervalMs = 40; // 25 FPS maximum

constexpr uint32_t logicalIndex(uint32_t x, uint32_t y) {
    return y * kLogicalWidth + x;
}

constexpr uint32_t rotatedPhysicalIndex(uint32_t logical_x, uint32_t logical_y) {
    const uint32_t physical_x = (kPhysicalWidth - 1U) - logical_y;
    const uint32_t physical_y = logical_x;
    return physical_y * kPhysicalWidth + physical_x;
}

static_assert(kLogicalPixels == 153600, "Unexpected logical framebuffer size");
static_assert(kPhysicalPixels == 153600, "Unexpected physical framebuffer size");
static_assert(logicalIndex(0, 0) == 0 && rotatedPhysicalIndex(0, 0) == 319, "Top-left rotation mismatch");
static_assert(logicalIndex(479, 0) == 479 && rotatedPhysicalIndex(479, 0) == 153599, "Top-right rotation mismatch");
static_assert(logicalIndex(0, 319) == 153120 && rotatedPhysicalIndex(0, 319) == 0, "Bottom-left rotation mismatch");
static_assert(logicalIndex(479, 319) == 153599 && rotatedPhysicalIndex(479, 319) == 153280, "Bottom-right rotation mismatch");

inline uint16_t color16(uint32_t color) { return static_cast<uint16_t>(color); }

inline bool inside(int32_t x, int32_t y) {
    return x >= 0 && x < static_cast<int32_t>(kLogicalWidth) &&
           y >= 0 && y < static_cast<int32_t>(kLogicalHeight);
}
}

tft_display::tft_display(int16_t w, int16_t h) : _width(480), _height(320) {
    (void)w;
    (void)h;
}

void tft_display::ensureFramebuffer() {
    if (_framebuffer && _physicalFramebuffer) return;
    _framebuffer = static_cast<uint16_t *>(
        heap_caps_malloc(kLogicalFramebufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
    );
    _physicalFramebuffer = static_cast<uint16_t *>(
        heap_caps_malloc(kPhysicalFramebufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
    );
    if (!_framebuffer || !_physicalFramebuffer) abort();
    memset(_framebuffer, 0, kLogicalFramebufferBytes);
    memset(_physicalFramebuffer, 0, kPhysicalFramebufferBytes);
    _dirty = true;
    Serial.println("[GFX] logical 480x320 + physical 320x480 framebuffers allocated in PSRAM");
}

void tft_display::begin(uint32_t speed) {
    (void)speed;
    ensureFramebuffer();
    static bool physical_ready = false;
    if (!physical_ready) {
        jc3248w535en_esp_lcd_init();
        physical_ready = true;
        Serial.println("[JC3248] physical display ready");
    }
    Serial.println("[GFX] JC3248 adapter ready 480x320");
}

void tft_display::init(uint8_t tc) {
    (void)tc;
    begin();
}

void tft_display::setRotation(uint8_t r) { _rotation = r; }

void tft_display::setPixel(int32_t x, int32_t y, uint16_t color) {
    if (!inside(x, y)) return;
    ensureFramebuffer();
    const uint32_t index = static_cast<uint32_t>(y) * kLogicalWidth + static_cast<uint32_t>(x);
    const uint16_t storedColor =
        _swapBytes ? static_cast<uint16_t>((color >> 8) | (color << 8)) : color;
    if (_framebuffer[index] == storedColor) return;
    _framebuffer[index] = storedColor;
    _dirty = true;
}

void tft_display::drawPixel(int32_t x, int32_t y, uint32_t color) { setPixel(x, y, color16(color)); }

void tft_display::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
    int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int32_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy;
    while (true) {
        setPixel(x0, y0, color16(color));
        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void tft_display::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) { fillRect(x, y, w, 1, color); }
void tft_display::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) { fillRect(x, y, 1, h, color); }

void tft_display::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    drawFastHLine(x, y, w, color); drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color); drawFastVLine(x + w - 1, y, h, color);
}

void tft_display::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    ensureFramebuffer();
    const int32_t x0 = max<int32_t>(0, x), y0 = max<int32_t>(0, y);
    const int32_t x1 = min<int32_t>(_width, x + w), y1 = min<int32_t>(_height, y + h);
    const uint16_t c = color16(color);
    for (int32_t yy = y0; yy < y1; ++yy)
        for (int32_t xx = x0; xx < x1; ++xx) setPixel(xx, yy, c);
}

void tft_display::fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t c1, uint32_t c2) {
    for (int32_t xx = 0; xx < w; ++xx) fillRect(x + xx, y, 1, h, xx * 2 < w ? c1 : c2);
}
void tft_display::fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t c1, uint32_t c2) {
    for (int32_t yy = 0; yy < h; ++yy) fillRect(x, y + yy, w, 1, yy * 2 < h ? c1 : c2);
}
void tft_display::fillScreen(uint32_t color) { fillRect(0, 0, _width, _height, color); }

void tft_display::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    r = max<int32_t>(0, min<int32_t>(r, min(w, h) / 2));
    if (r == 0) {
        drawRect(x, y, w, h, color);
        return;
    }

    drawFastHLine(x + r, y, w - 2 * r, color);
    drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
    drawFastVLine(x, y + r, h - 2 * r, color);
    drawFastVLine(x + w - 1, y + r, h - 2 * r, color);

    const int32_t top = y + r;
    const int32_t bottom = y + h - r - 1;
    const int32_t left = x + r;
    const int32_t right = x + w - r - 1;
    for (int32_t angle = 0; angle <= 90; ++angle) {
        const float radians = static_cast<float>(angle) * 0.0174532925f;
        const int32_t dx = lroundf(cosf(radians) * r);
        const int32_t dy = lroundf(sinf(radians) * r);
        setPixel(left - dx, top - dy, color16(color));
        setPixel(right + dx, top - dy, color16(color));
        setPixel(left - dx, bottom + dy, color16(color));
        setPixel(right + dx, bottom + dy, color16(color));
    }
}
void tft_display::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    r = max<int32_t>(0, min<int32_t>(r, min(w, h) / 2));
    if (r == 0) {
        fillRect(x, y, w, h, color);
        return;
    }
    fillRect(x + r, y, w - 2 * r, h, color); fillRect(x, y + r, w, h - 2 * r, color);
    fillCircle(x + r, y + r, r, color); fillCircle(x + w - r - 1, y + r, r, color);
    fillCircle(x + r, y + h - r - 1, r, color); fillCircle(x + w - r - 1, y + h - r - 1, r, color);
}

void tft_display::drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
    int32_t x = -r, y = 0, err = 2 - 2 * r;
    do {
        setPixel(x0 - x, y0 + y, color16(color)); setPixel(x0 - y, y0 - x, color16(color));
        setPixel(x0 + x, y0 - y, color16(color)); setPixel(x0 + y, y0 + x, color16(color));
        r = err; if (r <= y) err += ++y * 2 + 1; if (r > x || err > y) err += ++x * 2 + 1;
    } while (x < 0);
}
void tft_display::fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
    for (int32_t yy = -r; yy <= r; ++yy) {
        int32_t span = static_cast<int32_t>(sqrtf(static_cast<float>(r * r - yy * yy)));
        drawFastHLine(x0 - span, y0 + yy, span * 2 + 1, color);
    }
}
void tft_display::drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
    for (int a = 0; a < 360; ++a) {
        float rad = a * 0.0174532925f; setPixel(x0 + lroundf(cosf(rad) * rx), y0 + lroundf(sinf(rad) * ry), color);
    }
}
void tft_display::fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
    for (int32_t yy = -ry; yy <= ry; ++yy) {
        int32_t span = static_cast<int32_t>(rx * sqrtf(max(0.0f, 1.0f - (float)(yy * yy) / (ry * ry))));
        drawFastHLine(x0 - span, y0 + yy, span * 2 + 1, color);
    }
}
void tft_display::drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t c) {
    drawLine(x0, y0, x1, y1, c); drawLine(x1, y1, x2, y2, c); drawLine(x2, y2, x0, y0, c);
}
void tft_display::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t c) {
    int32_t minx = min(x0, min(x1, x2)), maxx = max(x0, max(x1, x2));
    int32_t miny = min(y0, min(y1, y2)), maxy = max(y0, max(y1, y2));
    const int32_t area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
    if (!area) return;
    for (int32_t y = miny; y <= maxy; ++y) for (int32_t x = minx; x <= maxx; ++x) {
        int32_t a = (x1 - x) * (y2 - y) - (x2 - x) * (y1 - y);
        int32_t b = (x2 - x) * (y0 - y) - (x0 - x) * (y2 - y);
        int32_t d = (x0 - x) * (y1 - y) - (x1 - x) * (y0 - y);
        if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0)) setPixel(x, y, color16(c));
    }
}
void tft_display::drawArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t start, uint32_t end, uint32_t fg, uint32_t bg, bool smooth) {
    (void)bg;
    (void)smooth;
    const int32_t outer = max(r, ir);
    const int32_t inner = max<int32_t>(0, min(r, ir));
    if (outer < 0) return;

    const uint32_t span = end >= start ? end - start : (360U - (start % 360U)) + (end % 360U);
    const uint32_t startAngle = start % 360U;
    const uint32_t endAngle = end % 360U;
    const int32_t outerSquared = outer * outer;
    const int32_t innerSquared = inner * inner;

    for (int32_t yy = -outer; yy <= outer; ++yy) {
        for (int32_t xx = -outer; xx <= outer; ++xx) {
            const int32_t distanceSquared = xx * xx + yy * yy;
            if (distanceSquared > outerSquared || distanceSquared < innerSquared) continue;

            float degrees = atan2f(static_cast<float>(yy), static_cast<float>(xx)) * 57.2957795f;
            if (degrees < 0.0f) degrees += 360.0f;
            const uint32_t angle = static_cast<uint32_t>(degrees);
            const bool insideAngle = span >= 360U ||
                (startAngle <= endAngle ? angle >= startAngle && angle <= endAngle
                                        : angle >= startAngle || angle <= endAngle);
            if (insideAngle) setPixel(x + xx, y + yy, color16(fg));
        }
    }
}
void tft_display::drawWideLine(float ax, float ay, float bx, float by, float wd, uint32_t fg, uint32_t bg) {
    (void)bg;
    if (wd <= 1.0f) {
        drawLine(lroundf(ax), lroundf(ay), lroundf(bx), lroundf(by), fg);
        return;
    }

    const float dx = bx - ax;
    const float dy = by - ay;
    const float length = sqrtf(dx * dx + dy * dy);
    if (length <= 0.0f) {
        fillCircle(lroundf(ax), lroundf(ay), static_cast<int32_t>(ceilf(wd * 0.5f)), fg);
        return;
    }

    const float halfWidth = wd * 0.5f;
    const float perpendicularX = -dy * halfWidth / length;
    const float perpendicularY = dx * halfWidth / length;
    const int32_t ax1 = lroundf(ax + perpendicularX);
    const int32_t ay1 = lroundf(ay + perpendicularY);
    const int32_t ax2 = lroundf(ax - perpendicularX);
    const int32_t ay2 = lroundf(ay - perpendicularY);
    const int32_t bx1 = lroundf(bx + perpendicularX);
    const int32_t by1 = lroundf(by + perpendicularY);
    const int32_t bx2 = lroundf(bx - perpendicularX);
    const int32_t by2 = lroundf(by - perpendicularY);

    fillTriangle(ax1, ay1, ax2, ay2, bx1, by1, fg);
    fillTriangle(ax2, ay2, bx2, by2, bx1, by1, fg);
    const int32_t capRadius = static_cast<int32_t>(ceilf(halfWidth));
    fillCircle(lroundf(ax), lroundf(ay), capRadius, fg);
    fillCircle(lroundf(bx), lroundf(by), capRadius, fg);
}

void tft_display::drawXBitmap(int16_t x, int16_t y, const uint8_t *bmp, int16_t w, int16_t h, uint16_t c) { drawXBitmap(x, y, bmp, w, h, c, TFT_BLACK); }
void tft_display::drawXBitmap(int16_t x, int16_t y, const uint8_t *bmp, int16_t w, int16_t h, uint16_t c, uint16_t bg) {
    if (!bmp) return;

    for (int16_t yy = 0; yy < h; ++yy) {
        for (int16_t xx = 0; xx < w; ++xx) {
            const bool foreground = bmp[yy * ((w + 7) / 8) + xx / 8] & (0x80 >> (xx & 7));
            setPixel(x + xx, y + yy, foreground ? c : bg);
        }
    }
}
void tft_display::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *d) { if (d) for (int32_t yy = 0; yy < h; ++yy) for (int32_t xx = 0; xx < w; ++xx) setPixel(x + xx, y + yy, d[yy * w + xx]); }
void tft_display::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *d) { pushImage(x, y, w, h, const_cast<const uint16_t *>(d)); }
void tft_display::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *d, bool bpp8, uint16_t *cmap) { if (d && bpp8 && cmap) for (int32_t yy = 0; yy < h; ++yy) for (int32_t xx = 0; xx < w; ++xx) setPixel(x + xx, y + yy, cmap[d[yy * w + xx]]); }
void tft_display::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *d, bool bpp8, uint16_t *cmap) { pushImage(x, y, w, h, const_cast<uint8_t *>(d), bpp8, cmap); }

void tft_display::setCursor(int16_t x, int16_t y) { _cursorX = x; _cursorY = y; }
int16_t tft_display::getCursorX() const { return _cursorX; }
int16_t tft_display::getCursorY() const { return _cursorY; }
void tft_display::setTextSize(uint8_t s) { _textSize = s ? s : 1; }
void tft_display::setTextColor(uint16_t c) { _textColor = c; }
void tft_display::setTextColor(uint16_t c, uint16_t b, bool bgfill) { (void)bgfill; _textColor = c; _textBgColor = b; }
void tft_display::setTextDatum(uint8_t d) { _textDatum = d; }
uint8_t tft_display::getTextDatum() const { return _textDatum; }
void tft_display::setTextFont(uint8_t f) { _textFont = f; }
void tft_display::setTextWrap(bool wrapX, bool wrapY) { (void)wrapY; _wrap = wrapX; }
int16_t tft_display::textWidth(const String &s, uint8_t fontId) const { (void)fontId; return s.length() * 6 * _textSize; }
int16_t tft_display::textWidth(const char *s, uint8_t fontId) const { (void)fontId; return s ? strlen(s) * 6 * _textSize : 0; }
int16_t tft_display::fontHeight(int16_t fontId) const { (void)fontId; return 8 * _textSize; }

void tft_display::drawGlyph(int32_t x, int32_t y, uint8_t c) {
    if (c < 32 || c > 127) c = '?';
    const uint8_t *glyph = &font[(c - 32) * 5];
    for (uint8_t col = 0; col < 5; ++col) for (uint8_t row = 0; row < 8; ++row) {
        const bool on = (glyph[col] >> row) & 1U;
        if (on || _textBgColor != TFT_TRANSPARENT) fillRect(x + col * _textSize, y + row * _textSize, _textSize, _textSize, on ? _textColor : _textBgColor);
    }
}
size_t tft_display::write(uint8_t c) {
    if (c == '\n') { _cursorX = 0; _cursorY += fontHeight(); return 1; }
    if (c == '\r') return 1;
    if (_wrap && _cursorX + 6 * _textSize > _width) { _cursorX = 0; _cursorY += fontHeight(); }
    drawGlyph(_cursorX, _cursorY, c); _cursorX += 6 * _textSize; return 1;
}
size_t tft_display::write(const uint8_t *buffer, size_t size) { if (!buffer) return 0; for (size_t i = 0; i < size; ++i) write(buffer[i]); return size; }
size_t tft_display::print(const String &s) { return write(reinterpret_cast<const uint8_t *>(s.c_str()), s.length()); }
size_t tft_display::println() { return write((const uint8_t *)"\n", 1); }
size_t tft_display::printf(const char *fmt, ...) { char b[256]; va_list ap; va_start(ap, fmt); int n = vsnprintf(b, sizeof(b), fmt, ap); va_end(ap); return n > 0 ? print(String(b)) : 0; }
int16_t tft_display::drawAlignedString(const String &s, int32_t x, int32_t y, uint8_t datum) {
    int32_t w = textWidth(s); if (datum == TC_DATUM || datum == MC_DATUM || datum == BC_DATUM) x -= w / 2; else if (datum == TR_DATUM || datum == MR_DATUM || datum == BR_DATUM) x -= w;
    if (datum == ML_DATUM || datum == MC_DATUM || datum == MR_DATUM) y -= fontHeight() / 2; else if (datum == BL_DATUM || datum == BC_DATUM || datum == BR_DATUM) y -= fontHeight();
    setCursor(x, y); return print(s);
}
int16_t tft_display::drawString(const String &s, int32_t x, int32_t y, uint8_t f) { (void)f; return drawAlignedString(s, x, y, _textDatum); }
int16_t tft_display::drawCentreString(const String &s, int32_t x, int32_t y, uint8_t f) { (void)f; return drawAlignedString(s, x, y, TC_DATUM); }
int16_t tft_display::drawRightString(const String &s, int32_t x, int32_t y, uint8_t f) { (void)f; return drawAlignedString(s, x, y, TR_DATUM); }

void tft_display::invertDisplay(bool i) { (void)i; }
void tft_display::sleep(bool value) { (void)value; }
void tft_display::setSwapBytes(bool swap) { _swapBytes = swap; }
bool tft_display::getSwapBytes() const { return _swapBytes; }
uint16_t tft_display::color565(uint8_t r, uint8_t g, uint8_t b) const { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
int16_t tft_display::width() const { return _width; }
int16_t tft_display::height() const { return _height; }
SPIClass &tft_display::getSPIinstance() const { return SPI; }
void tft_display::writecommand(uint8_t c) { (void)c; }
uint32_t tft_display::getTextColor() const { return _textColor; }
uint32_t tft_display::getTextBgColor() const { return _textBgColor; }
uint8_t tft_display::getTextSize() const { return _textSize; }
uint8_t tft_display::getRotation() const { return _rotation; }
Arduino_GFX *tft_display::native() { return nullptr; }
void tft_display::present() {
    ensureFramebuffer();
    if (!_dirty) return;

    const uint32_t now = millis();
    if (_lastPresentMs != 0 && now - _lastPresentMs < kMinimumPresentIntervalMs) return;

    const uint32_t presentStartUs = micros();

    // Rotate the complete logical frame before starting any DMA transfer.
    // Source stride is 480; destination stride is 320.
    for (uint32_t logical_y = 0; logical_y < kLogicalHeight; ++logical_y) {
        for (uint32_t logical_x = 0; logical_x < kLogicalWidth; ++logical_x) {
            const uint32_t logical_index = logicalIndex(logical_x, logical_y);
            const uint32_t physical_index = rotatedPhysicalIndex(logical_x, logical_y);
            _physicalFramebuffer[physical_index] = _framebuffer[logical_index];
        }
    }
    const uint32_t rotationUs = micros() - presentStartUs;

    static bool indices_logged = false;
    if (!indices_logged) {
        Serial.println("[GFX] logical 0,0 idx=0 -> physical 319,0 idx=319");
        Serial.println("[GFX] logical 479,0 idx=479 -> physical 319,479 idx=153599");
        Serial.println("[GFX] logical 0,319 idx=153120 -> physical 0,0 idx=0");
        Serial.println("[GFX] logical 479,319 idx=153599 -> physical 0,479 idx=153280");
        indices_logged = true;
    }

    const uint32_t transferStartUs = micros();
    jc3248w535en_esp_lcd_present_physical(_physicalFramebuffer);
    const uint32_t transferUs = micros() - transferStartUs;
    const uint32_t totalUs = micros() - presentStartUs;

    _dirty = false;
    _lastPresentMs = millis();

    static uint8_t presentLogCount = 0;
    if (presentLogCount < 5) {
        Serial.printf(
            "[GFX] present frame %u rotate=%u us tx=%u us total=%u us\n",
            static_cast<unsigned>(presentLogCount + 1),
            static_cast<unsigned>(rotationUs),
            static_cast<unsigned>(transferUs),
            static_cast<unsigned>(totalUs)
        );
        ++presentLogCount;
    }
}

// Sprite compatibility uses a private software buffer and the same drawing
// primitives, avoiding any direct panel access.
tft_sprite::tft_sprite(tft_display *parent) : _display(parent) {}
void *tft_sprite::createSprite(int16_t w, int16_t h, uint8_t frames) { (void)frames; _width = w; _height = h; _buffer.assign((size_t)w * h, TFT_BLACK); return _buffer.data(); }
void tft_sprite::deleteSprite() { _buffer.clear(); _width = _height = 0; }
bool tft_sprite::_hasBuffer() const { return !_buffer.empty(); }
void tft_sprite::setPixel(int32_t x, int32_t y, uint16_t c) { if (_hasBuffer() && x >= 0 && y >= 0 && x < _width && y < _height) _buffer[y * _width + x] = c; }
void tft_sprite::fillScreen(uint32_t c) { fillRect(0, 0, _width, _height, c); }
void tft_sprite::setColorDepth(uint8_t d) { _colorDepth = d; }
void tft_sprite::setCursor(int16_t x, int16_t y) { _cursorX = x; _cursorY = y; }
void tft_sprite::setTextColor(uint16_t c) { _textColor = c; }
void tft_sprite::setTextColor(uint16_t c, uint16_t b) { _textColor = c; _textBgColor = b; }
void tft_sprite::setTextSize(uint8_t s) { _textSize = s ? s : 1; }
void tft_sprite::setTextDatum(uint8_t d) { _textDatum = d; }
void tft_sprite::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) { for (int32_t yy = y; yy < y + h; ++yy) for (int32_t xx = x; xx < x + w; ++xx) setPixel(xx, yy, color16(c)); }
void tft_sprite::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t c) { fillRect(x, y, 1, h, c); }
void tft_sprite::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t c) { if (_display) { const int32_t steps = abs(x1 - x0) + abs(y1 - y0); for (int32_t i = 0; i <= steps; ++i) { float t = (float)i / (float)max<int32_t>(1, steps); setPixel(lroundf(x0 + (x1 - x0) * t), lroundf(y0 + (y1 - y0) * t), color16(c)); } } }
void tft_sprite::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) { drawLine(x,y,x+w-1,y,c); drawLine(x,y+h-1,x+w-1,y+h-1,c); drawLine(x,y,x,y+h-1,c); drawLine(x+w-1,y,x+w-1,y+h-1,c); }
void tft_sprite::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t c) {
    if (w <= 0 || h <= 0) return;
    r = max<int32_t>(0, min<int32_t>(r, min(w, h) / 2));
    if (r == 0) {
        drawRect(x, y, w, h, c);
        return;
    }
    fillRect(x + r, y, w - 2 * r, 1, c);
    fillRect(x + r, y + h - 1, w - 2 * r, 1, c);
    fillRect(x, y + r, 1, h - 2 * r, c);
    fillRect(x + w - 1, y + r, 1, h - 2 * r, c);
    const int32_t top = y + r, bottom = y + h - r - 1;
    const int32_t left = x + r, right = x + w - r - 1;
    for (int32_t angle = 0; angle <= 90; ++angle) {
        const float radians = static_cast<float>(angle) * 0.0174532925f;
        const int32_t dx = lroundf(cosf(radians) * r);
        const int32_t dy = lroundf(sinf(radians) * r);
        setPixel(left - dx, top - dy, color16(c));
        setPixel(right + dx, top - dy, color16(c));
        setPixel(left - dx, bottom + dy, color16(c));
        setPixel(right + dx, bottom + dy, color16(c));
    }
}
void tft_sprite::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t c) {
    if (w <= 0 || h <= 0) return;
    r = max<int32_t>(0, min<int32_t>(r, min(w, h) / 2));
    if (r == 0) {
        fillRect(x, y, w, h, c);
        return;
    }
    fillRect(x + r, y, w - 2 * r, h, c);
    fillRect(x, y + r, w, h - 2 * r, c);
    fillCircle(x + r, y + r, r, c);
    fillCircle(x + w - r - 1, y + r, r, c);
    fillCircle(x + r, y + h - r - 1, r, c);
    fillCircle(x + w - r - 1, y + h - r - 1, r, c);
}
void tft_sprite::drawPixel(int32_t x,int32_t y,uint32_t c){setPixel(x,y,color16(c));}
void tft_sprite::fillCircle(int32_t x,int32_t y,int32_t r,uint32_t c){for(int yy=-r;yy<=r;++yy){int s=(int)sqrtf((float)(r*r-yy*yy));fillRect(x-s,y+yy,2*s+1,1,c);}}
void tft_sprite::drawCircle(int32_t x,int32_t y,int32_t r,uint32_t c){for(int a=0;a<360;++a){float q=a*.0174532925f;setPixel(x+lroundf(cosf(q)*r),y+lroundf(sinf(q)*r),color16(c));}}
void tft_sprite::fillEllipse(int16_t x,int16_t y,int32_t rx,int32_t ry,uint16_t c){for(int yy=-ry;yy<=ry;++yy){int s=(int)(rx*sqrtf(max(0.f,1.f-(float)(yy*yy)/(ry*ry))));fillRect(x-s,y+yy,2*s+1,1,c);}}
void tft_sprite::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t c) {
    const int32_t minX = min(x0, min(x1, x2));
    const int32_t maxX = max(x0, max(x1, x2));
    const int32_t minY = min(y0, min(y1, y2));
    const int32_t maxY = max(y0, max(y1, y2));
    const int32_t area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
    if (area == 0) return;
    for (int32_t yy = minY; yy <= maxY; ++yy) {
        for (int32_t xx = minX; xx <= maxX; ++xx) {
            const int32_t a = (x1 - xx) * (y2 - yy) - (x2 - xx) * (y1 - yy);
            const int32_t b = (x2 - xx) * (y0 - yy) - (x0 - xx) * (y2 - yy);
            const int32_t d = (x0 - xx) * (y1 - yy) - (x1 - xx) * (y0 - yy);
            if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0)) {
                setPixel(xx, yy, color16(c));
            }
        }
    }
}
void tft_sprite::pushSprite(int32_t x,int32_t y,uint32_t transparent){if(!_display||!_hasBuffer())return;for(int yy=0;yy<_height;++yy)for(int xx=0;xx<_width;++xx)if(_buffer[yy*_width+xx]!=transparent)_display->drawPixel(x+xx,y+yy,_buffer[yy*_width+xx]);}
void tft_sprite::pushToSprite(tft_sprite *dest,int32_t x,int32_t y,uint32_t transparent){if(!dest||!_hasBuffer())return;for(int yy=0;yy<_height;++yy)for(int xx=0;xx<_width;++xx)if(_buffer[yy*_width+xx]!=transparent)dest->setPixel(x+xx,y+yy,_buffer[yy*_width+xx]);}
int16_t tft_sprite::width() const{return _width;} int16_t tft_sprite::height() const{return _height;} int16_t tft_sprite::fontHeight(int16_t f) const{(void)f;return 8*_textSize;}
void tft_sprite::drawXBitmap(int16_t x,int16_t y,const uint8_t*b,int16_t w,int16_t h,uint16_t c,uint16_t bg){if(!b)return;for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx)setPixel(x+xx,y+yy,(b[yy*((w+7)/8)+xx/8]&(0x80>>(xx&7)))?c:bg);}
void tft_sprite::pushImage(int32_t x,int32_t y,int32_t w,int32_t h,const uint16_t*d){if(d)for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx)setPixel(x+xx,y+yy,d[yy*w+xx]);}
void tft_sprite::pushImage(int32_t x,int32_t y,int32_t w,int32_t h,uint8_t*d,bool b,uint16_t*c){if(d&&b&&c)for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx)setPixel(x+xx,y+yy,c[d[yy*w+xx]]);}
void tft_sprite::pushImage(int32_t x,int32_t y,int32_t w,int32_t h,const uint8_t*d,bool b,uint16_t*c){pushImage(x,y,w,h,const_cast<uint8_t*>(d),b,c);}
void tft_sprite::fillRectHGradient(int16_t x,int16_t y,int16_t w,int16_t h,uint32_t c1,uint32_t c2){fillRect(x,y,w/2,h,c1);fillRect(x+w/2,y,w-w/2,h,c2);} void tft_sprite::fillRectVGradient(int16_t x,int16_t y,int16_t w,int16_t h,uint32_t c1,uint32_t c2){fillRect(x,y,w,h/2,c1);fillRect(x,y+h/2,w,h-h/2,c2);}
int16_t tft_sprite::drawString(const String &s, int32_t x, int32_t y, uint8_t f) {
    (void)f;
    const int32_t textWidth = static_cast<int32_t>(s.length()) * 6 * _textSize;
    const int32_t textHeight = 8 * _textSize;
    if (_textDatum == TC_DATUM || _textDatum == MC_DATUM || _textDatum == BC_DATUM) x -= textWidth / 2;
    else if (_textDatum == TR_DATUM || _textDatum == MR_DATUM || _textDatum == BR_DATUM) x -= textWidth;
    if (_textDatum == ML_DATUM || _textDatum == MC_DATUM || _textDatum == MR_DATUM) y -= textHeight / 2;
    else if (_textDatum == BL_DATUM || _textDatum == BC_DATUM || _textDatum == BR_DATUM) y -= textHeight;

    int32_t cursorX = x;
    for (size_t i = 0; i < s.length(); ++i) {
        uint8_t character = static_cast<uint8_t>(s[i]);
        if (character < 32 || character > 127) character = '?';
        const uint8_t *glyph = &font[(character - 32) * 5];
        for (uint8_t column = 0; column < 5; ++column) {
            for (uint8_t row = 0; row < 8; ++row) {
                const bool foreground = (glyph[column] >> row) & 1U;
                if (foreground || _textBgColor != TFT_TRANSPARENT) {
                    fillRect(
                        cursorX + column * _textSize,
                        y + row * _textSize,
                        _textSize,
                        _textSize,
                        foreground ? _textColor : _textBgColor
                    );
                }
            }
        }
        cursorX += 6 * _textSize;
    }
    _cursorX = cursorX;
    _cursorY = y;
    return textWidth;
}
