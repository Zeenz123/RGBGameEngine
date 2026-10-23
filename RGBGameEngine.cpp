#include "RGBGameEngine.h"
#include "waveshare_lcd_port.h"
#include "PixelConverter.h"
#include <algorithm>
#include <math.h>
#include <string.h>
#include "esp_heap_caps.h"

using std::swap;
RGBGameEngine *RGBGameEngine::instance = nullptr;

// ============================================================================
// Standard 5x7 ASCII Bitmap Font (Characters 32 ' ' to 126 '~')
// Each character is 5 columns wide, LSB = top pixel
// ============================================================================
static const uint8_t FONT5X7[95][5] PROGMEM = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7F,0x14,0x7F,0x14}, {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00}, {0x00,0x1C,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1C,0x00}, {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02}, {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31}, {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00}, {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06}, {0x32,0x49,0x79,0x41,0x3E},
    {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00}, {0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40}, {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20}, {0x38,0x44,0x44,0x48,0x7F},
    {0x38,0x54,0x54,0x54,0x18}, {0x08,0x7E,0x09,0x01,0x02}, {0x0C,0x52,0x52,0x52,0x3E},
    {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00}, {0x20,0x40,0x44,0x3D,0x00},
    {0x7F,0x10,0x28,0x44,0x00}, {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38}, {0x7C,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7C}, {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C}, {0x1C,0x20,0x40,0x20,0x1C},
    {0x3C,0x40,0x30,0x40,0x3C}, {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00}, {0x00,0x00,0x7F,0x00,0x00},
    {0x00,0x41,0x36,0x08,0x00}, {0x10,0x08,0x08,0x10,0x08}
};

uint16_t* RGBGameEngine::getDrawBuffer()
{
    return frameBuffer;
}

RGBGameEngine::RGBGameEngine()
{
    frontBuffer = nullptr;
    backBuffer = nullptr;
    frameBuffer = nullptr;

    refreshFinished = true;
    swapPending = false;

    lcd = nullptr;

    display.width = 800;
    display.height = 480;
    display.bitsPerPixel = 16;
}

bool RGBGameEngine::begin(PixelFormat format)
{
    pixelFormat = format;
    waveshare_lcd_init();
    lcd = ::lcd;

    if (lcd == nullptr)
    {
        Serial0.println("RGBGameEngine: LCD INIT FAILED");
        return false;
    }

    instance = this;
    registerRefreshCallback(RGBGameEngine::refreshCallback);

    frontBuffer = (uint16_t *)lcd->getFrameBufferByIndex(0);
    backBuffer  = (uint16_t *)lcd->getFrameBufferByIndex(1);

    if (frontBuffer == nullptr || backBuffer == nullptr)
    {
        Serial0.println("RGBGameEngine: DRIVER FRAMEBUFFER FAILED");
        return false;
    }

    frameBuffer = backBuffer;
    refreshFinished = true;
    swapPending = false;

    const size_t pixelCount = (size_t)display.width * display.height;
    for (size_t i = 0; i < pixelCount; i++)
    {
        frontBuffer[i] = 0x0000;
        backBuffer[i]  = 0x0000;
    }

    Serial0.println("RGBGameEngine: DOUBLE BUFFER ENABLED & READY");
    return true;
}

uint16_t *RGBGameEngine::getFrameBuffer()
{
    return frameBuffer;
}

uint16_t RGBGameEngine::getPixel(int x, int y) const
{
    if (x < 0 || x >= display.width || y < 0 || y >= display.height) return 0x0000;
    return frameBuffer[y * display.width + x];
}

int RGBGameEngine::width() const  { return display.width; }
int RGBGameEngine::height() const { return display.height; }

void RGBGameEngine::clear(uint16_t color)
{
    const int total = display.width * display.height;
    uint32_t c32 = ((uint32_t)color << 16) | color;
    uint32_t *p32 = (uint32_t *)frameBuffer;
    int half = total >> 1;
    for (int i = 0; i < half; i++) {
        p32[i] = c32;
    }
}

void RGBGameEngine::clear(Color color)
{
    clear(PixelConverter::toRGB565(color));
}

// ============================================================================
// 1. PIXELS, TRANSPARENCY & BLEND MODES
// ============================================================================

void RGBGameEngine::drawPixel(int x, int y, uint16_t color)
{
    if (x < 0 || x >= display.width || y < 0 || y >= display.height) return;
    frameBuffer[y * display.width + x] = color;
}

void RGBGameEngine::drawPixel(int x, int y, Color color)
{
    drawPixel(x, y, PixelConverter::toRGB565(color));
}

void RGBGameEngine::drawPixelAlpha(int x, int y, uint16_t color, uint8_t alpha)
{
    if (x < 0 || x >= display.width || y < 0 || y >= display.height) return;
    int idx = y * display.width + x;
    frameBuffer[idx] = Color::blend565(color, frameBuffer[idx], alpha);
}

void RGBGameEngine::drawPixelBlend(int x, int y, uint16_t color, uint8_t alpha, BlendMode mode)
{
    if (x < 0 || x >= display.width || y < 0 || y >= display.height) return;
    int idx = y * display.width + x;
    frameBuffer[idx] = Color::applyBlendMode565(color, frameBuffer[idx], alpha, mode);
}

// ============================================================================
// 2. LINES (FAST, THICK, DASHED, XIAOLIN WU ANTI-ALIASED)
// ============================================================================

void RGBGameEngine::drawFastHLine(int16_t x, int16_t y, int16_t width, uint16_t color)
{
    if (width <= 0 || y < 0 || y >= display.height) return;
    if (x < 0) { width += x; x = 0; }
    if (x + width > display.width) { width = display.width - x; }
    if (width <= 0) return;

    uint16_t *dst = &frameBuffer[y * display.width + x];

    // 32-bit aligned dual-pixel PSRAM burst writes (2x memory throughput!)
    if (((uintptr_t)dst & 2U) && width > 0) {
        *dst++ = color;
        width--;
    }
    uint32_t c32 = ((uint32_t)color << 16) | color;
    uint32_t *dst32 = (uint32_t *)dst;
    int16_t pairs = width >> 1;
    for (int16_t i = 0; i < pairs; i++) {
        dst32[i] = c32;
    }
    if (width & 1) {
        dst[pairs << 1] = color;
    }
}

void RGBGameEngine::drawFastHLine(int16_t x, int16_t y, int16_t width, Color color)
{
    drawFastHLine(x, y, width, PixelConverter::toRGB565(color));
}

void RGBGameEngine::drawFastHLineAlpha(int16_t x, int16_t y, int16_t width, uint16_t color, uint8_t alpha)
{
    if (alpha < 4) return;
    if (alpha >= 252) { drawFastHLine(x, y, width, color); return; }
    if (width <= 0 || y < 0 || y >= display.height) return;
    if (x < 0) { width += x; x = 0; }
    if (x + width > display.width) { width = display.width - x; }
    if (width <= 0) return;

    uint32_t a = ((uint32_t)alpha + 4U) >> 3; // 1..31
    uint32_t invA = 32U - a;
    uint16_t *dst = &frameBuffer[y * display.width + x];

    // Smooth 32-level darkness path for black shadows (color == 0x0000)
    if (color == 0x0000) {
        for (int16_t i = 0; i < width; i++) {
            uint32_t d = dst[i];
            uint32_t r = (((d >> 11) & 0x1FU) * invA) >> 5;
            uint32_t g = (((d >> 5)  & 0x3FU) * invA) >> 5;
            uint32_t b = (( d        & 0x1FU) * invA) >> 5;
            dst[i] = (uint16_t)((r << 11) | (g << 5) | b);
        }
        return;
    }

    // Pre-multiply foreground R, G, B channels once outside the scanline loop!
    uint32_t sr_a = ((color >> 11) & 0x1FU) * a;
    uint32_t sg_a = ((color >> 5)  & 0x3FU) * a;
    uint32_t sb_a = ( color        & 0x1FU) * a;

    for (int16_t i = 0; i < width; i++) {
        uint32_t d = dst[i];
        uint32_t r = (sr_a + (((d >> 11) & 0x1FU) * invA)) >> 5;
        uint32_t g = (sg_a + (((d >> 5)  & 0x3FU) * invA)) >> 5;
        uint32_t b = (sb_a + (( d        & 0x1FU) * invA)) >> 5;
        dst[i] = (uint16_t)((r << 11) | (g << 5) | b);
    }
}

void RGBGameEngine::drawFastHLineBlend(int16_t x, int16_t y, int16_t width, uint16_t color, uint8_t alpha, BlendMode mode)
{
    if (mode == BlendMode::SOLID) {
        drawFastHLine(x, y, width, color);
        return;
    }
    if (mode == BlendMode::ALPHA) {
        drawFastHLineAlpha(x, y, width, color, alpha);
        return;
    }
    if (width <= 0 || y < 0 || y >= display.height || alpha == 0) return;
    if (x < 0) { width += x; x = 0; }
    if (x + width > display.width) { width = display.width - x; }
    if (width <= 0) return;

    uint16_t *dst = &frameBuffer[y * display.width + x];

    // 1. Fast inline scanline path for ADDITIVE glow
    if (mode == BlendMode::ADDITIVE) {
        uint16_t scaled = Color::scaleBrightness565(color, alpha);
        uint16_t sr = (scaled >> 11) & 0x1F;
        uint16_t sg = (scaled >> 5)  & 0x3F;
        uint16_t sb =  scaled        & 0x1F;
        for (int16_t i = 0; i < width; i++) {
            uint16_t d = dst[i];
            uint16_t r = ((d >> 11) & 0x1F) + sr; if (r > 31) r = 31;
            uint16_t g = ((d >> 5)  & 0x3F) + sg; if (g > 63) g = 63;
            uint16_t b = ( d        & 0x1F) + sb; if (b > 31) b = 31;
            dst[i] = (r << 11) | (g << 5) | b;
        }
        return;
    }

    // 2. Fast inline scanline path for SUBTRACTIVE energy absorption
    if (mode == BlendMode::SUBTRACTIVE) {
        uint16_t scaled = Color::scaleBrightness565(color, alpha);
        int16_t sr = (scaled >> 11) & 0x1F;
        int16_t sg = (scaled >> 5)  & 0x3F;
        int16_t sb =  scaled        & 0x1F;
        for (int16_t i = 0; i < width; i++) {
            uint16_t d = dst[i];
            int16_t r = (int16_t)((d >> 11) & 0x1F) - sr; if (r < 0) r = 0;
            int16_t g = (int16_t)((d >> 5)  & 0x3F) - sg; if (g < 0) g = 0;
            int16_t b = (int16_t)( d        & 0x1F) - sb; if (b < 0) b = 0;
            dst[i] = ((uint16_t)r << 11) | ((uint16_t)g << 5) | (uint16_t)b;
        }
        return;
    }

    // 3. Fast inline scanline path for MULTIPLY optical stained-glass filter
    if (mode == BlendMode::MULTIPLY) {
        uint32_t a = ((uint32_t)alpha + 4U) >> 3; // 1..32
        uint32_t invA = 32U - a;
        uint32_t sr = (color >> 11) & 0x1FU;
        uint32_t sg = (color >> 5)  & 0x3FU;
        uint32_t sb =  color        & 0x1FU;
        for (int16_t i = 0; i < width; i++) {
            uint32_t d = dst[i];
            uint32_t dr = (d >> 11) & 0x1FU;
            uint32_t dg = (d >> 5)  & 0x3FU;
            uint32_t db =  d        & 0x1FU;
            uint32_t mr = (dr * (sr + 1U)) >> 5;
            uint32_t mg = (dg * (sg + 1U)) >> 6;
            uint32_t mb = (db * (sb + 1U)) >> 5;
            uint32_t r = (mr * a + dr * invA) >> 5;
            uint32_t g = (mg * a + dg * invA) >> 5;
            uint32_t b = (mb * a + db * invA) >> 5;
            dst[i] = (uint16_t)((r << 11) | (g << 5) | b);
        }
        return;
    }

    // 4. Fast inline 16-level ordered Bayer stipple screen-door transparency
    if (mode == BlendMode::CHECKER_STIPPLE) {
        static const uint8_t bayer4[4][4] = {
            {   0, 128,  32, 160 },
            { 192,  64, 224,  96 },
            {  48, 176,  16, 144 },
            { 240, 112, 208,  80 }
        };
        const uint8_t *bRow = bayer4[y & 3];
        for (int16_t i = 0; i < width; i++) {
            if (alpha > bRow[(x + i) & 3]) {
                dst[i] = color;
            }
        }
        return;
    }

    for (int16_t i = 0; i < width; i++) {
        dst[i] = Color::applyBlendMode565(color, dst[i], alpha, mode);
    }
}

void RGBGameEngine::drawFastVLine(int16_t x, int16_t y, int16_t height, uint16_t color)
{
    if (height <= 0 || x < 0 || x >= display.width) return;
    if (y < 0) { height += y; y = 0; }
    if (y + height > display.height) { height = display.height - y; }
    if (height <= 0) return;

    uint16_t *dst = &frameBuffer[y * display.width + x];
    for (int16_t i = 0; i < height; i++) {
        *dst = color;
        dst += display.width;
    }
}

void RGBGameEngine::drawFastVLine(int16_t x, int16_t y, int16_t height, Color color)
{
    drawFastVLine(x, y, height, PixelConverter::toRGB565(color));
}

void RGBGameEngine::drawLine(int x0, int y0, int x1, int y1, uint16_t color)
{
    if (y0 == y1) {
        int left = (x0 < x1) ? x0 : x1;
        drawFastHLine(left, y0, abs(x1 - x0) + 1, color);
        return;
    }
    if (x0 == x1) {
        int top = (y0 < y1) ? y0 : y1;
        drawFastVLine(x0, top, abs(y1 - y0) + 1, color);
        return;
    }

    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void RGBGameEngine::drawLine(int x0, int y0, int x1, int y1, Color color)
{
    drawLine(x0, y0, x1, y1, PixelConverter::toRGB565(color));
}

void RGBGameEngine::drawLineAlpha(int x0, int y0, int x1, int y1, uint16_t color, uint8_t alpha)
{
    int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        drawPixelAlpha(x0, y0, color, alpha);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void RGBGameEngine::drawThickLine(int x0, int y0, int x1, int y1, int16_t thickness, uint16_t color, bool roundCaps)
{
    if (thickness <= 1) {
        drawLine(x0, y0, x1, y1, color);
        return;
    }
    float dx = (float)(x1 - x0);
    float dy = (float)(y1 - y0);
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) {
        if (roundCaps) fillCircle(x0, y0, thickness / 2, color);
        return;
    }
    float nx = -dy / len * (thickness * 0.5f);
    float ny =  dx / len * (thickness * 0.5f);

    int16_t ax = (int16_t)(x0 + nx), ay = (int16_t)(y0 + ny);
    int16_t bx = (int16_t)(x0 - nx), by = (int16_t)(y0 - ny);
    int16_t cx = (int16_t)(x1 - nx), cy = (int16_t)(y1 - ny);
    int16_t dxp = (int16_t)(x1 + nx), dyp = (int16_t)(y1 + ny);

    fillTriangle(ax, ay, bx, by, cx, cy, color);
    fillTriangle(ax, ay, cx, cy, dxp, dyp, color);

    if (roundCaps) {
        fillCircle(x0, y0, thickness / 2, color);
        fillCircle(x1, y1, thickness / 2, color);
    }
}

void RGBGameEngine::drawDashedLine(int x0, int y0, int x1, int y1, int16_t dashLen, int16_t gapLen, uint16_t color)
{
    if (dashLen <= 0) dashLen = 4;
    if (gapLen < 0) gapLen = 4;
    int period = dashLen + gapLen;

    int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int step = 0;

    while (true) {
        if ((step % period) < dashLen) {
            drawPixel(x0, y0, color);
        }
        step++;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void RGBGameEngine::drawAntiAliasedLine(int x0, int y0, int x1, int y1, uint16_t color)
{
    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) { swap(x0, y0); swap(x1, y1); }
    if (x0 > x1) { swap(x0, x1); swap(y0, y1); }

    int dx = x1 - x0;
    int dy = y1 - y0;
    float gradient = (dx == 0) ? 1.0f : (float)dy / (float)dx;

    float intery = (float)y0;
    for (int x = x0; x <= x1; x++) {
        int iy = (int)floorf(intery);
        float frac = intery - (float)iy;
        uint8_t a1 = (uint8_t)((1.0f - frac) * 255.0f);
        uint8_t a2 = (uint8_t)(frac * 255.0f);

        if (steep) {
            drawPixelAlpha(iy,     x, color, a1);
            drawPixelAlpha(iy + 1, x, color, a2);
        } else {
            drawPixelAlpha(x, iy,     color, a1);
            drawPixelAlpha(x, iy + 1, color, a2);
        }
        intery += gradient;
    }
}

// ============================================================================
// 3. RECTANGLES, ROUNDED RECTS & QUADS
// ============================================================================

void RGBGameEngine::drawRect(int x, int y, int width, int height, uint16_t color)
{
    if (width <= 0 || height <= 0) return;
    drawFastHLine(x, y, width, color);
    if (height > 1) drawFastHLine(x, y + height - 1, width, color);
    if (height > 2) {
        drawFastVLine(x, y + 1, height - 2, color);
        if (width > 1) drawFastVLine(x + width - 1, y + 1, height - 2, color);
    }
}

void RGBGameEngine::drawRect(int x, int y, int width, int height, Color color)
{
    drawRect(x, y, width, height, PixelConverter::toRGB565(color));
}

void RGBGameEngine::drawThickRect(int x, int y, int width, int height, int16_t border, uint16_t color)
{
    for (int16_t i = 0; i < border; i++) {
        drawRect(x + i, y + i, width - 2 * i, height - 2 * i, color);
    }
}

void RGBGameEngine::fillRect(int x, int y, int width, int height, uint16_t color)
{
    if (width <= 0 || height <= 0) return;
    for (int yy = y; yy < y + height; yy++) {
        drawFastHLine(x, yy, width, color);
    }
}

void RGBGameEngine::fillRect(int x, int y, int width, int height, Color color)
{
    fillRect(x, y, width, height, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillRectAlpha(int x, int y, int width, int height, uint16_t color, uint8_t alpha)
{
    if (width <= 0 || height <= 0 || alpha == 0) return;
    for (int yy = y; yy < y + height; yy++) {
        drawFastHLineAlpha(x, yy, width, color, alpha);
    }
}

void RGBGameEngine::fillRectBlend(int x, int y, int width, int height, uint16_t color, uint8_t alpha, BlendMode mode)
{
    if (width <= 0 || height <= 0 || alpha == 0) return;
    for (int yy = y; yy < y + height; yy++) {
        drawFastHLineBlend(x, yy, width, color, alpha, mode);
    }
}

void RGBGameEngine::drawCircleCorner(int16_t centerX, int16_t centerY, int16_t radius, uint8_t cornerMask, uint16_t color)
{
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        if (cornerMask & 0x4) { drawPixel(centerX + x, centerY + y, color); drawPixel(centerX + y, centerY + x, color); }
        if (cornerMask & 0x2) { drawPixel(centerX + x, centerY - y, color); drawPixel(centerX + y, centerY - x, color); }
        if (cornerMask & 0x8) { drawPixel(centerX - y, centerY + x, color); drawPixel(centerX - x, centerY + y, color); }
        if (cornerMask & 0x1) { drawPixel(centerX - y, centerY - x, color); drawPixel(centerX - x, centerY - y, color); }
    }
}

void RGBGameEngine::fillCircleCorner(int16_t centerX, int16_t centerY, int16_t radius, uint8_t cornerMask, int16_t delta, uint16_t color)
{
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        if (cornerMask & 0x1) {
            drawFastVLine(centerX + x, centerY - y, 2 * y + 1 + delta, color);
            drawFastVLine(centerX + y, centerY - x, 2 * x + 1 + delta, color);
        }
        if (cornerMask & 0x2) {
            drawFastVLine(centerX - x, centerY - y, 2 * y + 1 + delta, color);
            drawFastVLine(centerX - y, centerY - x, 2 * x + 1 + delta, color);
        }
    }
}

void RGBGameEngine::drawRoundRect(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color)
{
    if (width <= 0 || height <= 0) return;
    int16_t maxR = ((width < height) ? width : height) / 2;
    if (radius > maxR) radius = maxR;
    if (radius <= 0) { drawRect(x, y, width, height, color); return; }

    drawFastHLine(x + radius, y, width - 2 * radius, color);
    drawFastHLine(x + radius, y + height - 1, width - 2 * radius, color);
    drawFastVLine(x, y + radius, height - 2 * radius, color);
    drawFastVLine(x + width - 1, y + radius, height - 2 * radius, color);

    drawCircleCorner(x + radius,             y + radius,              radius, 1, color);
    drawCircleCorner(x + width - radius - 1, y + radius,              radius, 2, color);
    drawCircleCorner(x + width - radius - 1, y + height - radius - 1, radius, 4, color);
    drawCircleCorner(x + radius,             y + height - radius - 1, radius, 8, color);
}

void RGBGameEngine::drawRoundRect(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, Color color)
{
    drawRoundRect(x, y, width, height, radius, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillRoundRect(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color)
{
    if (width <= 0 || height <= 0) return;
    int16_t maxR = ((width < height) ? width : height) / 2;
    if (radius > maxR) radius = maxR;
    if (radius <= 0) { fillRect(x, y, width, height, color); return; }

    fillRect(x + radius, y, width - 2 * radius, height, color);
    fillCircleCorner(x + width - radius - 1, y + radius, radius, 1, height - 2 * radius - 1, color);
    fillCircleCorner(x + radius,             y + radius, radius, 2, height - 2 * radius - 1, color);
}

void RGBGameEngine::fillRoundRect(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, Color color)
{
    fillRoundRect(x, y, width, height, radius, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillRoundRectAlpha(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color, uint8_t alpha)
{
    fillRoundRectBlend(x, y, width, height, radius, color, alpha, BlendMode::ALPHA);
}

void RGBGameEngine::fillRoundRectBlend(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color, uint8_t alpha, BlendMode mode)
{
    if (width <= 0 || height <= 0 || alpha == 0) return;
    int16_t maxR = ((width < height) ? width : height) / 2;
    if (radius > maxR) radius = maxR;

    for (int16_t yy = 0; yy < height; yy++) {
        int16_t inset = 0;
        if (yy < radius) {
            int16_t dy = radius - yy;
            inset = radius - (int16_t)sqrtf((float)(radius * radius - dy * dy));
        } else if (yy >= height - radius) {
            int16_t dy = yy - (height - radius - 1);
            inset = radius - (int16_t)sqrtf((float)(radius * radius - dy * dy));
        }
        drawFastHLineBlend(x + inset, y + yy, width - 2 * inset, color, alpha, mode);
    }
}

void RGBGameEngine::fillVerticalAlphaGradientRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color, uint8_t topAlpha, uint8_t bottomAlpha)
{
    if (width <= 0 || height <= 0) return;
    for (int16_t yy = 0; yy < height; yy++) {
        uint8_t a = (uint8_t)(topAlpha + ((int32_t)(bottomAlpha - topAlpha) * yy) / max((int16_t)1, (int16_t)(height - 1)));
        drawFastHLineAlpha(x, y + yy, width, color, a);
    }
}

void RGBGameEngine::drawGlassLensCircle(int16_t cx, int16_t cy, int16_t radius, uint16_t tintColor, uint8_t alpha, float refractionStrength)
{
    if (radius <= 2) return;
    (void)refractionStrength;
    // Render a crystal-clear translucent glass lens with soft outer rim & bright specular reflection arc
    fillCircleAlpha(cx, cy, radius, tintColor, alpha);
    fillCircleAlpha(cx - radius / 4, cy - radius / 4, radius / 2, Colors565::White, alpha / 2);
    drawCircle(cx, cy, radius, Colors565::White);
    drawCircle(cx, cy, radius - 1, Colors565::LightSkyBlue);
    drawArc(cx, cy, radius - 5, 200.0f, 260.0f, Colors565::White);
    drawArc(cx, cy, radius - 6, 200.0f, 260.0f, Colors565::White);
}

void RGBGameEngine::drawQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color)
{
    Point p[4] = { {x0, y0}, {x1, y1}, {x2, y2}, {x3, y3} };
    drawPolygon(p, 4, color);
}

void RGBGameEngine::fillQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color)
{
    fillTriangle(x0, y0, x1, y1, x2, y2, color);
    fillTriangle(x0, y0, x2, y2, x3, y3, color);
}

void RGBGameEngine::fillDiamond(int16_t centerX, int16_t centerY, int16_t halfW, int16_t halfH, uint16_t color)
{
    fillQuad(centerX, centerY - halfH, centerX + halfW, centerY, centerX, centerY + halfH, centerX - halfW, centerY, color);
}

// ============================================================================
// 4. POLYLINES, POLYGONS (SCANLINE FILL), STARS & N-GONS
// ============================================================================

void RGBGameEngine::drawPolyline(const Point *points, uint16_t pointCount, uint16_t color)
{
    if (pointCount < 2) return;
    for (uint16_t i = 0; i < pointCount - 1; i++) {
        drawLine(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y, color);
    }
}

void RGBGameEngine::drawPolyline(const Point *points, uint16_t pointCount, Color color)
{
    drawPolyline(points, pointCount, PixelConverter::toRGB565(color));
}

void RGBGameEngine::drawPolygon(const Point *points, uint16_t pointCount, uint16_t color)
{
    if (pointCount < 2) return;
    drawPolyline(points, pointCount, color);
    drawLine(points[pointCount - 1].x, points[pointCount - 1].y, points[0].x, points[0].y, color);
}

void RGBGameEngine::drawPolygon(const Point *points, uint16_t pointCount, Color color)
{
    drawPolygon(points, pointCount, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillPolygon(const Point *points, uint16_t pointCount, uint16_t color)
{
    fillPolygonAlpha(points, pointCount, color, 255);
}

void RGBGameEngine::fillPolygonAlpha(const Point *points, uint16_t pointCount, uint16_t color, uint8_t alpha)
{
    if (pointCount < 3 || alpha == 0) return;

    int16_t minY = points[0].y, maxY = points[0].y;
    for (uint16_t i = 1; i < pointCount; i++) {
        if (points[i].y < minY) minY = points[i].y;
        if (points[i].y > maxY) maxY = points[i].y;
    }
    if (minY < 0) minY = 0;
    if (maxY >= display.height) maxY = display.height - 1;

    int16_t nodeX[64];
    uint16_t maxNodes = (pointCount < 64) ? pointCount : 64;

    for (int16_t y = minY; y <= maxY; y++) {
        int nodes = 0;
        int j = pointCount - 1;
        for (int i = 0; i < (int)pointCount; i++) {
            if ((points[i].y < y && points[j].y >= y) || (points[j].y < y && points[i].y >= y)) {
                if (nodes < (int)maxNodes) {
                    nodeX[nodes++] = (int16_t)(points[i].x + (float)(y - points[i].y) / (float)(points[j].y - points[i].y) * (points[j].x - points[i].x));
                }
            }
            j = i;
        }
        // Insertion sort nodeX
        for (int i = 1; i < nodes; i++) {
            int16_t key = nodeX[i];
            int k = i - 1;
            while (k >= 0 && nodeX[k] > key) {
                nodeX[k + 1] = nodeX[k];
                k--;
            }
            nodeX[k + 1] = key;
        }
        for (int i = 0; i < nodes - 1; i += 2) {
            if (nodeX[i + 1] >= nodeX[i]) {
                if (alpha == 255)
                    drawFastHLine(nodeX[i], y, nodeX[i + 1] - nodeX[i] + 1, color);
                else
                    drawFastHLineAlpha(nodeX[i], y, nodeX[i + 1] - nodeX[i] + 1, color, alpha);
            }
        }
    }
}

void RGBGameEngine::drawRegularPolygon(int16_t cx, int16_t cy, uint8_t sides, int16_t radius, float rotationDeg, uint16_t color)
{
    if (sides < 3 || sides > 32) return;
    Point pts[32];
    float rad0 = rotationDeg * 0.0174532925f;
    float step = 6.2831853f / sides;
    for (uint8_t i = 0; i < sides; i++) {
        float a = rad0 + i * step;
        pts[i] = Point(cx + (int16_t)(cosf(a) * radius), cy + (int16_t)(sinf(a) * radius));
    }
    drawPolygon(pts, sides, color);
}

void RGBGameEngine::fillRegularPolygon(int16_t cx, int16_t cy, uint8_t sides, int16_t radius, float rotationDeg, uint16_t color)
{
    if (sides < 3 || sides > 32) return;
    Point pts[32];
    float rad0 = rotationDeg * 0.0174532925f;
    float step = 6.2831853f / sides;
    for (uint8_t i = 0; i < sides; i++) {
        float a = rad0 + i * step;
        pts[i] = Point(cx + (int16_t)(cosf(a) * radius), cy + (int16_t)(sinf(a) * radius));
    }
    fillPolygon(pts, sides, color);
}

void RGBGameEngine::drawStar(int16_t cx, int16_t cy, uint8_t points, int16_t outerRadius, int16_t innerRadius, float rotationDeg, uint16_t color)
{
    if (points < 3 || points > 16) return;
    Point pts[32];
    uint8_t total = points * 2;
    float rad0 = (rotationDeg - 90.0f) * 0.0174532925f;
    float step = 3.14159265f / points;
    for (uint8_t i = 0; i < total; i++) {
        float r = (i & 1) ? innerRadius : outerRadius;
        float a = rad0 + i * step;
        pts[i] = Point(cx + (int16_t)(cosf(a) * r), cy + (int16_t)(sinf(a) * r));
    }
    drawPolygon(pts, total, color);
}

void RGBGameEngine::fillStar(int16_t cx, int16_t cy, uint8_t points, int16_t outerRadius, int16_t innerRadius, float rotationDeg, uint16_t color)
{
    if (points < 3 || points > 16) return;
    Point pts[32];
    uint8_t total = points * 2;
    float rad0 = (rotationDeg - 90.0f) * 0.0174532925f;
    float step = 3.14159265f / points;
    for (uint8_t i = 0; i < total; i++) {
        float r = (i & 1) ? innerRadius : outerRadius;
        float a = rad0 + i * step;
        pts[i] = Point(cx + (int16_t)(cosf(a) * r), cy + (int16_t)(sinf(a) * r));
    }
    fillPolygon(pts, total, color);
}

// ============================================================================
// 5. TRIANGLES (SOLID, ALPHA & GOURAUD SHADED)
// ============================================================================

void RGBGameEngine::drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color)
{
    Point p[3] = { { x1, y1 }, { x2, y2 }, { x3, y3 } };
    drawPolygon(p, 3, color);
}

void RGBGameEngine::drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Color color)
{
    drawTriangle(x1, y1, x2, y2, x3, y3, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color)
{
    fillTriangleAlpha(x1, y1, x2, y2, x3, y3, color, 255);
}

void RGBGameEngine::fillTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Color color)
{
    fillTriangle(x1, y1, x2, y2, x3, y3, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillTriangleAlpha(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color, uint8_t alpha)
{
    if (alpha == 0) return;
    if (y1 > y2) { swap(y1, y2); swap(x1, x2); }
    if (y1 > y3) { swap(y1, y3); swap(x1, x3); }
    if (y2 > y3) { swap(y2, y3); swap(x2, x3); }

    if (y1 == y3) {
        int16_t minX = min(x1, min(x2, x3));
        int16_t maxX = max(x1, max(x2, x3));
        if (alpha == 255) drawFastHLine(minX, y1, maxX - minX + 1, color);
        else drawFastHLineAlpha(minX, y1, maxX - minX + 1, color, alpha);
        return;
    }

    float dx13 = (y3 != y1) ? (float)(x3 - x1) / (float)(y3 - y1) : 0.0f;
    float dx12 = (y2 != y1) ? (float)(x2 - x1) / (float)(y2 - y1) : 0.0f;
    float dx23 = (y3 != y2) ? (float)(x3 - x2) / (float)(y3 - y2) : 0.0f;

    float startX = (float)x1;
    float endX   = (float)x1;

    for (int16_t y = y1; y < y2; y++) {
        int16_t left  = (int16_t)startX;
        int16_t right = (int16_t)endX;
        if (left > right) swap(left, right);
        if (alpha == 255) drawFastHLine(left, y, right - left + 1, color);
        else drawFastHLineAlpha(left, y, right - left + 1, color, alpha);
        startX += dx13;
        endX   += dx12;
    }

    startX = (float)x1 + dx13 * (float)(y2 - y1);
    endX   = (float)x2;

    for (int16_t y = y2; y <= y3; y++) {
        int16_t left  = (int16_t)startX;
        int16_t right = (int16_t)endX;
        if (left > right) swap(left, right);
        if (alpha == 255) drawFastHLine(left, y, right - left + 1, color);
        else drawFastHLineAlpha(left, y, right - left + 1, color, alpha);
        startX += dx13;
        endX   += dx23;
    }
}

void RGBGameEngine::fillTriangleGouraud(int16_t x1, int16_t y1, uint16_t c1, int16_t x2, int16_t y2, uint16_t c2, int16_t x3, int16_t y3, uint16_t c3)
{
    int16_t minX = max((int16_t)0, min(x1, min(x2, x3)));
    int16_t maxX = min((int16_t)(display.width - 1), max(x1, max(x2, x3)));
    int16_t minY = max((int16_t)0, min(y1, min(y2, y3)));
    int16_t maxY = min((int16_t)(display.height - 1), max(y1, max(y2, y3)));

    int32_t denom = (int32_t)(y2 - y3) * (x1 - x3) + (int32_t)(x3 - x2) * (y1 - y3);
    if (denom == 0) return;
    float invDenom = 1.0f / (float)denom;

    Color col1 = Color::fromRGB565(c1);
    Color col2 = Color::fromRGB565(c2);
    Color col3 = Color::fromRGB565(c3);

    for (int16_t y = minY; y <= maxY; y++) {
        uint16_t *row = &frameBuffer[y * display.width];
        for (int16_t x = minX; x <= maxX; x++) {
            float w1 = ((float)(y2 - y3) * (x - x3) + (float)(x3 - x2) * (y - y3)) * invDenom;
            float w2 = ((float)(y3 - y1) * (x - x3) + (float)(x1 - x3) * (y - y3)) * invDenom;
            float w3 = 1.0f - w1 - w2;
            if (w1 >= 0.0f && w2 >= 0.0f && w3 >= 0.0f) {
                uint8_t r = (uint8_t)(w1 * col1.r + w2 * col2.r + w3 * col3.r);
                uint8_t g = (uint8_t)(w1 * col1.g + w2 * col2.g + w3 * col3.g);
                uint8_t b = (uint8_t)(w1 * col1.b + w2 * col2.b + w3 * col3.b);
                row[x] = Color::ditherRGBto565(r, g, b, x, y);
            }
        }
    }
}

// ============================================================================
// 6. CIRCLES, RINGS, ELLIPSES, ARCS & CAPSULES
// ============================================================================

void RGBGameEngine::drawCirclePoints(int16_t centerX, int16_t centerY, int16_t x, int16_t y, uint16_t color)
{
    if (x == y) {
        drawPixel(centerX + x, centerY + y, color);
        drawPixel(centerX - x, centerY + y, color);
        drawPixel(centerX + x, centerY - y, color);
        drawPixel(centerX - x, centerY - y, color);
        return;
    }
    drawPixel(centerX + x, centerY + y, color);
    drawPixel(centerX - x, centerY + y, color);
    drawPixel(centerX + x, centerY - y, color);
    drawPixel(centerX - x, centerY - y, color);
    drawPixel(centerX + y, centerY + x, color);
    drawPixel(centerX - y, centerY + x, color);
    drawPixel(centerX + y, centerY - x, color);
    drawPixel(centerX - y, centerY - x, color);
}

void RGBGameEngine::drawEllipsePoints(int16_t centerX, int16_t centerY, int16_t x, int16_t y, uint16_t color)
{
    drawPixel(centerX + x, centerY + y, color);
    drawPixel(centerX - x, centerY + y, color);
    drawPixel(centerX + x, centerY - y, color);
    drawPixel(centerX - x, centerY - y, color);
}

void RGBGameEngine::drawCircle(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color)
{
    if (radius < 0) return;
    if (radius == 0) { drawPixel(centerX, centerY, color); return; }

    int16_t x = 0, y = radius;
    int16_t d = 3 - (2 * radius);

    while (x <= y) {
        drawCirclePoints(centerX, centerY, x, y, color);
        if (d < 0) {
            d = d + (4 * x) + 6;
        } else {
            d = d + (4 * (x - y)) + 10;
            y--;
        }
        x++;
    }
}

void RGBGameEngine::drawCircle(int16_t x, int16_t y, int16_t radius, Color color)
{
    drawCircle(x, y, radius, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillCircle(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color)
{
    if (radius < 0) return;
    if (radius == 0) { drawPixel(centerX, centerY, color); return; }

    int16_t x = 0, y = radius;
    int16_t d = 3 - (2 * radius);
    while (x <= y) {
        drawFastHLine(centerX - x, centerY + y, (2 * x) + 1, color);
        drawFastHLine(centerX - x, centerY - y, (2 * x) + 1, color);
        if (x != y) {
            drawFastHLine(centerX - y, centerY + x, (2 * y) + 1, color);
            drawFastHLine(centerX - y, centerY - x, (2 * y) + 1, color);
        }
        if (d < 0) {
            d += (4 * x) + 6;
        } else {
            d += (4 * (x - y)) + 10;
            y--;
        }
        x++;
    }
}

void RGBGameEngine::fillCircle(int16_t centerX, int16_t centerY, int16_t radius, Color color)
{
    fillCircle(centerX, centerY, radius, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillCircleAlpha(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color, uint8_t alpha)
{
    fillCircleBlend(centerX, centerY, radius, color, alpha, BlendMode::ALPHA);
}

void RGBGameEngine::fillCircleBlend(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color, uint8_t alpha, BlendMode mode)
{
    if (radius <= 0 || alpha == 0) return;
    int32_t r2 = (int32_t)radius * radius;
    for (int16_t dy = -radius; dy <= radius; dy++) {
        int16_t span = (int16_t)sqrtf((float)(r2 - dy * dy));
        drawFastHLineBlend(centerX - span, centerY + dy, 2 * span + 1, color, alpha, mode);
    }
}

void RGBGameEngine::drawRing(int16_t cx, int16_t cy, int16_t innerRadius, int16_t outerRadius, uint16_t color)
{
    drawCircle(cx, cy, innerRadius, color);
    drawCircle(cx, cy, outerRadius, color);
}

void RGBGameEngine::fillRing(int16_t cx, int16_t cy, int16_t innerRadius, int16_t outerRadius, uint16_t color)
{
    if (outerRadius <= 0 || innerRadius >= outerRadius) return;
    if (innerRadius <= 0) { fillCircle(cx, cy, outerRadius, color); return; }

    int32_t rOut2 = (int32_t)outerRadius * outerRadius;
    int32_t rIn2  = (int32_t)innerRadius * innerRadius;

    for (int16_t dy = -outerRadius; dy <= outerRadius; dy++) {
        int32_t dy2 = (int32_t)dy * dy;
        int16_t xOut = (int16_t)sqrtf((float)(rOut2 - dy2));
        if (abs(dy) >= innerRadius) {
            drawFastHLine(cx - xOut, cy + dy, 2 * xOut + 1, color);
        } else {
            int16_t xIn = (int16_t)sqrtf((float)(rIn2 - dy2));
            drawFastHLine(cx - xOut, cy + dy, xOut - xIn + 1, color);
            drawFastHLine(cx + xIn,  cy + dy, xOut - xIn + 1, color);
        }
    }
}

void RGBGameEngine::drawEllipse(int16_t centerX, int16_t centerY, int16_t radiusX, int16_t radiusY, uint16_t color)
{
    if (radiusX < 0 || radiusY < 0) return;
    if (radiusX == 0 && radiusY == 0) { drawPixel(centerX, centerY, color); return; }
    if (radiusX == 0) { drawFastVLine(centerX, centerY - radiusY, (2 * radiusY) + 1, color); return; }
    if (radiusY == 0) { drawFastHLine(centerX - radiusX, centerY, (2 * radiusX) + 1, color); return; }

    int32_t rx2 = (int32_t)radiusX * radiusX;
    int32_t ry2 = (int32_t)radiusY * radiusY;
    int32_t twoRx2 = 2 * rx2, twoRy2 = 2 * ry2;
    int32_t x = 0, y = radiusY;
    int32_t px = 0, py = twoRx2 * y;

    drawEllipsePoints(centerX, centerY, x, y, color);
    int32_t p = ry2 - (rx2 * radiusY) + (rx2 / 4);
    while (px < py) {
        x++; px += twoRy2;
        if (p < 0) p += ry2 + px;
        else { y--; py -= twoRx2; p += ry2 + px - py; }
        drawEllipsePoints(centerX, centerY, x, y, color);
    }

    p = (int32_t)(ry2 * (x + 0.5f) * (x + 0.5f)) + (rx2 * (y - 1) * (y - 1)) - (rx2 * ry2);
    while (y > 0) {
        y--; py -= twoRx2;
        if (p > 0) p += rx2 - py;
        else { x++; px += twoRy2; p += rx2 - py + px; }
        drawEllipsePoints(centerX, centerY, x, y, color);
    }
}

void RGBGameEngine::drawEllipse(int16_t centerX, int16_t centerY, int16_t radiusX, int16_t radiusY, Color color)
{
    drawEllipse(centerX, centerY, radiusX, radiusY, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillEllipse(int16_t centerX, int16_t centerY, int16_t radiusX, int16_t radiusY, uint16_t color)
{
    if (radiusX <= 0 || radiusY <= 0) return;
    for (int16_t dy = -radiusY; dy <= radiusY; dy++) {
        float norm = (float)dy / (float)radiusY;
        int16_t span = (int16_t)(radiusX * sqrtf(max(0.0f, 1.0f - norm * norm)));
        drawFastHLine(centerX - span, centerY + dy, 2 * span + 1, color);
    }
}

void RGBGameEngine::fillEllipse(int16_t centerX, int16_t centerY, int16_t radiusX, int16_t radiusY, Color color)
{
    fillEllipse(centerX, centerY, radiusX, radiusY, PixelConverter::toRGB565(color));
}

void RGBGameEngine::drawArc(int16_t centerX, int16_t centerY, int16_t radius, float startAngleDeg, float endAngleDeg, uint16_t color)
{
    if (radius <= 0) return;
    if (endAngleDeg < startAngleDeg) swap(startAngleDeg, endAngleDeg);
    float step = 1.0f / (float)radius;
    float startRad = startAngleDeg * 0.0174532925f;
    float endRad   = endAngleDeg   * 0.0174532925f;

    for (float a = startRad; a <= endRad; a += step) {
        drawPixel(centerX + (int16_t)(cosf(a) * radius), centerY + (int16_t)(sinf(a) * radius), color);
    }
}

void RGBGameEngine::drawArc(int16_t centerX, int16_t centerY, int16_t radius, float startAngleDeg, float endAngleDeg, Color color)
{
    drawArc(centerX, centerY, radius, startAngleDeg, endAngleDeg, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillArc(int16_t centerX, int16_t centerY, int16_t radius, float startAngleDeg, float endAngleDeg, uint16_t color)
{
    fillArcRing(centerX, centerY, 0, radius, startAngleDeg, endAngleDeg, color);
}

void RGBGameEngine::fillArc(int16_t centerX, int16_t centerY, int16_t radius, float startAngleDeg, float endAngleDeg, Color color)
{
    fillArc(centerX, centerY, radius, startAngleDeg, endAngleDeg, PixelConverter::toRGB565(color));
}

void RGBGameEngine::fillArcRing(int16_t centerX, int16_t centerY, int16_t innerRadius, int16_t outerRadius, float startAngleDeg, float endAngleDeg, uint16_t color)
{
    if (outerRadius <= 0 || innerRadius >= outerRadius) return;
    if (endAngleDeg < startAngleDeg) swap(startAngleDeg, endAngleDeg);

    int segments = (int)((endAngleDeg - startAngleDeg) * 0.35f) + 3;
    if (segments > 90) segments = 90;

    float stepRad = (endAngleDeg - startAngleDeg) * 0.0174532925f / segments;
    float a0 = startAngleDeg * 0.0174532925f;

    for (int i = 0; i < segments; i++) {
        float a1 = a0 + stepRad;
        float c0 = cosf(a0), s0 = sinf(a0);
        float c1 = cosf(a1), s1 = sinf(a1);

        int16_t ox0 = centerX + (int16_t)(c0 * outerRadius);
        int16_t oy0 = centerY + (int16_t)(s0 * outerRadius);
        int16_t ox1 = centerX + (int16_t)(c1 * outerRadius);
        int16_t oy1 = centerY + (int16_t)(s1 * outerRadius);

        if (innerRadius <= 0) {
            fillTriangle(centerX, centerY, ox0, oy0, ox1, oy1, color);
        } else {
            int16_t ix0 = centerX + (int16_t)(c0 * innerRadius);
            int16_t iy0 = centerY + (int16_t)(s0 * innerRadius);
            int16_t ix1 = centerX + (int16_t)(c1 * innerRadius);
            int16_t iy1 = centerY + (int16_t)(s1 * innerRadius);
            fillQuad(ix0, iy0, ox0, oy0, ox1, oy1, ix1, iy1, color);
        }
        a0 = a1;
    }
}

void RGBGameEngine::drawCapsule(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t radius, uint16_t color)
{
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0);
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) { drawCircle(x0, y0, radius, color); return; }

    float nx = -dy / len * radius;
    float ny =  dx / len * radius;

    drawLine((int16_t)(x0 + nx), (int16_t)(y0 + ny), (int16_t)(x1 + nx), (int16_t)(y1 + ny), color);
    drawLine((int16_t)(x0 - nx), (int16_t)(y0 - ny), (int16_t)(x1 - nx), (int16_t)(y1 - ny), color);
    drawCircle(x0, y0, radius, color);
    drawCircle(x1, y1, radius, color);
}

void RGBGameEngine::fillCapsule(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t radius, uint16_t color)
{
    drawThickLine(x0, y0, x1, y1, radius * 2, color, true);
}

// ============================================================================
// 7. CURVES, SPLINES, ARROWS & 2.5D WIREFRAMES
// ============================================================================

void RGBGameEngine::drawQuadBezier(int16_t x0, int16_t y0, int16_t cx, int16_t cy, int16_t x1, int16_t y1, uint16_t color, int16_t thickness)
{
    const int steps = 32;
    int16_t px = x0, py = y0;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / steps;
        float u = 1.0f - t;
        int16_t nx = (int16_t)(u * u * x0 + 2.0f * u * t * cx + t * t * x1);
        int16_t ny = (int16_t)(u * u * y0 + 2.0f * u * t * cy + t * t * y1);
        drawThickLine(px, py, nx, ny, thickness, color, true);
        px = nx; py = ny;
    }
}

void RGBGameEngine::drawCubicBezier(int16_t x0, int16_t y0, int16_t cx0, int16_t cy0, int16_t cx1, int16_t cy1, int16_t x1, int16_t y1, uint16_t color, int16_t thickness)
{
    const int steps = 40;
    int16_t px = x0, py = y0;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / steps;
        float u = 1.0f - t;
        float b0 = u * u * u;
        float b1 = 3.0f * u * u * t;
        float b2 = 3.0f * u * t * t;
        float b3 = t * t * t;
        int16_t nx = (int16_t)(b0 * x0 + b1 * cx0 + b2 * cx1 + b3 * x1);
        int16_t ny = (int16_t)(b0 * y0 + b1 * cy0 + b2 * cy1 + b3 * y1);
        drawThickLine(px, py, nx, ny, thickness, color, true);
        px = nx; py = ny;
    }
}

void RGBGameEngine::drawCatmullRomSpline(const Point *points, uint16_t count, uint16_t color, int16_t thickness)
{
    if (count < 4) return;
    for (uint16_t i = 0; i < count - 3; i++) {
        const Point &p0 = points[i];
        const Point &p1 = points[i + 1];
        const Point &p2 = points[i + 2];
        const Point &p3 = points[i + 3];

        int16_t px = p1.x, py = p1.y;
        const int steps = 16;
        for (int s = 1; s <= steps; s++) {
            float t = (float)s / steps;
            float t2 = t * t, t3 = t2 * t;
            float fx = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t +
                       (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                       (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
            float fy = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t +
                       (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                       (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
            int16_t nx = (int16_t)fx, ny = (int16_t)fy;
            drawThickLine(px, py, nx, ny, thickness, color, true);
            px = nx; py = ny;
        }
    }
}

void RGBGameEngine::drawArrow(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t headSize, uint16_t color, int16_t thickness)
{
    drawThickLine(x0, y0, x1, y1, thickness, color, true);
    float angle = atan2f((float)(y1 - y0), (float)(x1 - x0));
    int16_t hx1 = x1 - (int16_t)(headSize * cosf(angle - 0.45f));
    int16_t hy1 = y1 - (int16_t)(headSize * sinf(angle - 0.45f));
    int16_t hx2 = x1 - (int16_t)(headSize * cosf(angle + 0.45f));
    int16_t hy2 = y1 - (int16_t)(headSize * sinf(angle + 0.45f));
    fillTriangle(x1, y1, hx1, hy1, hx2, hy2, color);
}

void RGBGameEngine::drawCrosshair(int16_t cx, int16_t cy, int16_t radius, uint16_t color)
{
    drawCircle(cx, cy, radius, color);
    drawCircle(cx, cy, radius / 2, color);
    drawFastHLine(cx - radius - 4, cy, 2 * radius + 9, color);
    drawFastVLine(cx, cy - radius - 4, 2 * radius + 9, color);
}

void RGBGameEngine::drawCube3D(int16_t centerX, int16_t centerY, float size, float rotX, float rotY, float rotZ, uint16_t color)
{
    float h = size * 0.5f;
    Vec3 v[8] = {
        {-h, -h, -h}, { h, -h, -h}, { h,  h, -h}, {-h,  h, -h},
        {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h}
    };
    Vec3 rv[8];
    Point p[8];
    for (int i = 0; i < 8; i++) {
        rv[i] = v[i].rotateX(rotX).rotateY(rotY).rotateZ(rotZ);
        // Use telephoto projection (950/950) so parallel cube edges stay strictly cubic without fisheye warping!
        p[i] = rv[i].project(centerX, centerY, 950.0f, 950.0f);
    }
    const uint8_t edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
    };
    uint16_t backCol = Color::scaleBrightness565(color, 115);

    // 1. Draw deeper back edges first (dimmer)
    for (int i = 0; i < 12; i++) {
        uint8_t a = edges[i][0], b = edges[i][1];
        if ((rv[a].z + rv[b].z) > 0.0f) {
            drawLine(p[a].x, p[a].y, p[b].x, p[b].y, backCol);
        }
    }
    // 2. Draw closer front edges on top (crisp 2px thick + vertex joints)
    for (int i = 0; i < 12; i++) {
        uint8_t a = edges[i][0], b = edges[i][1];
        if ((rv[a].z + rv[b].z) <= 0.0f) {
            drawThickLine(p[a].x, p[a].y, p[b].x, p[b].y, 2, color, true);
        }
    }
    for (int i = 0; i < 8; i++) {
        fillCircle(p[i].x, p[i].y, (rv[i].z <= 0.0f) ? 3 : 2, (rv[i].z <= 0.0f) ? Colors565::White : backCol);
    }
}

void RGBGameEngine::drawPyramid3D(int16_t centerX, int16_t centerY, float size, float rotX, float rotY, float rotZ, uint16_t color)
{
    float h = size * 0.5f;
    Vec3 v[5] = {
        { 0, -h,  0},
        {-h,  h, -h}, { h,  h, -h}, { h,  h,  h}, {-h,  h,  h}
    };
    Point p[5];
    for (int i = 0; i < 5; i++) {
        p[i] = v[i].rotateX(rotX).rotateY(rotY).rotateZ(rotZ).project(centerX, centerY);
    }
    for (int i = 1; i <= 4; i++) {
        drawLine(p[0].x, p[0].y, p[i].x, p[i].y, color);
        int next = (i % 4) + 1;
        drawLine(p[i].x, p[i].y, p[next].x, p[next].y, color);
    }
}

void RGBGameEngine::drawSphereGrid3D(int16_t centerX, int16_t centerY, float radius, float rotX, float rotY, uint16_t color)
{
    const int rings = 6, sectors = 12;
    for (int r = 1; r < rings; r++) {
        float phi = 3.14159265f * r / rings;
        Point prev;
        for (int s = 0; s <= sectors; s++) {
            float theta = 6.2831853f * s / sectors;
            Vec3 v(radius * sinf(phi) * cosf(theta), radius * cosf(phi), radius * sinf(phi) * sinf(theta));
            Point pt = v.rotateX(rotX).rotateY(rotY).project(centerX, centerY);
            if (s > 0) drawLine(prev.x, prev.y, pt.x, pt.y, color);
            prev = pt;
        }
    }
}

// ============================================================================
// 8. GRADIENTS & DROP SHADOWS (WITH 4x4 BAYER ORDERED DITHERING)
// ============================================================================

void RGBGameEngine::fillLinearGradientRect(int16_t x, int16_t y, int16_t w, int16_t h, Color startColor, Color endColor, bool vertical, bool dither)
{
    if (w <= 0 || h <= 0) return;

    if (vertical) {
        for (int16_t row = 0; row < h; row++) {
            int16_t sy = y + row;
            if (sy < 0 || sy >= display.height) continue;
            uint8_t r = startColor.r + ((int32_t)(endColor.r - startColor.r) * row) / max((int16_t)1, (int16_t)(h - 1));
            uint8_t g = startColor.g + ((int32_t)(endColor.g - startColor.g) * row) / max((int16_t)1, (int16_t)(h - 1));
            uint8_t b = startColor.b + ((int32_t)(endColor.b - startColor.b) * row) / max((int16_t)1, (int16_t)(h - 1));

            if (!dither) {
                drawFastHLine(x, sy, w, RGB565_CONST(r, g, b));
            } else {
                int16_t xStart = max((int16_t)0, x);
                int16_t xEnd   = min((int16_t)display.width, (int16_t)(x + w));
                uint16_t *dst  = &frameBuffer[sy * display.width];
                for (int16_t sx = xStart; sx < xEnd; sx++) {
                    dst[sx] = Color::ditherRGBto565(r, g, b, sx, sy);
                }
            }
        }
    } else {
        int16_t xStart = max((int16_t)0, x);
        int16_t xEnd   = min((int16_t)display.width, (int16_t)(x + w));
        for (int16_t row = 0; row < h; row++) {
            int16_t sy = y + row;
            if (sy < 0 || sy >= display.height) continue;
            uint16_t *dst = &frameBuffer[sy * display.width];
            for (int16_t sx = xStart; sx < xEnd; sx++) {
                int16_t col = sx - x;
                uint8_t r = startColor.r + ((int32_t)(endColor.r - startColor.r) * col) / max((int16_t)1, (int16_t)(w - 1));
                uint8_t g = startColor.g + ((int32_t)(endColor.g - startColor.g) * col) / max((int16_t)1, (int16_t)(w - 1));
                uint8_t b = startColor.b + ((int32_t)(endColor.b - startColor.b) * col) / max((int16_t)1, (int16_t)(w - 1));
                dst[sx] = dither ? Color::ditherRGBto565(r, g, b, sx, sy) : RGB565_CONST(r, g, b);
            }
        }
    }
}

void RGBGameEngine::fillDiagonalGradientRect(int16_t x, int16_t y, int16_t w, int16_t h, Color topLeft, Color bottomRight, bool dither)
{
    if (w <= 0 || h <= 0) return;
    int32_t denom = max(1, (int)(w + h - 2));
    int16_t xStart = max((int16_t)0, x);
    int16_t xEnd   = min((int16_t)display.width, (int16_t)(x + w));
    int16_t yStart = max((int16_t)0, y);
    int16_t yEnd   = min((int16_t)display.height, (int16_t)(y + h));

    for (int16_t sy = yStart; sy < yEnd; sy++) {
        uint16_t *dst = &frameBuffer[sy * display.width];
        int16_t ry = sy - y;
        for (int16_t sx = xStart; sx < xEnd; sx++) {
            int32_t t = (sx - x) + ry;
            uint8_t r = topLeft.r + ((int32_t)(bottomRight.r - topLeft.r) * t) / denom;
            uint8_t g = topLeft.g + ((int32_t)(bottomRight.g - topLeft.g) * t) / denom;
            uint8_t b = topLeft.b + ((int32_t)(bottomRight.b - topLeft.b) * t) / denom;
            dst[sx] = dither ? Color::ditherRGBto565(r, g, b, sx, sy) : RGB565_CONST(r, g, b);
        }
    }
}

void RGBGameEngine::fillRadialGradientCircle(int16_t cx, int16_t cy, int16_t radius, Color innerColor, Color outerColor, bool dither)
{
    if (radius <= 0) return;
    int32_t r2 = (int32_t)radius * radius;
    int32_t dr = (int32_t)outerColor.r - innerColor.r;
    int32_t dg = (int32_t)outerColor.g - innerColor.g;
    int32_t db = (int32_t)outerColor.b - innerColor.b;

    for (int16_t dy = -radius; dy <= radius; dy++) {
        int16_t sy = cy + dy;
        if (sy < 0 || sy >= display.height) continue;
        int32_t dy2 = (int32_t)dy * dy;
        int16_t span = (int16_t)sqrtf((float)(r2 - dy2));
        uint16_t *dst = &frameBuffer[sy * display.width];

        for (int16_t dx = -span; dx <= span; dx++) {
            int16_t sx = cx + dx;
            if (sx < 0 || sx >= display.width) continue;
            // Fast integer quadratic ratio (0..256), zero sqrtf per pixel!
            int32_t t256 = ((dx * dx + dy2) << 8) / r2;
            if (t256 > 256) t256 = 256;

            uint8_t r = (uint8_t)(innerColor.r + ((dr * t256) >> 8));
            uint8_t g = (uint8_t)(innerColor.g + ((dg * t256) >> 8));
            uint8_t b = (uint8_t)(innerColor.b + ((db * t256) >> 8));
            dst[sx] = dither ? Color::ditherRGBto565(r, g, b, sx, sy) : RGB565_CONST(r, g, b);
        }
    }
}

void RGBGameEngine::drawDropShadowRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, int16_t offsetX, int16_t offsetY, int16_t blurPasses, uint8_t shadowAlpha)
{
    if (blurPasses < 1) blurPasses = 1;
    uint8_t passAlpha = shadowAlpha / blurPasses;
    for (int16_t i = blurPasses; i >= 1; i--) {
        int16_t expand = i * 2;
        fillRoundRectAlpha(x + offsetX - expand, y + offsetY - expand, w + expand * 2, h + expand * 2, radius + expand, 0x0000, passAlpha);
    }
}

void RGBGameEngine::drawDropShadowCircle(int16_t cx, int16_t cy, int16_t radius, int16_t offsetX, int16_t offsetY, int16_t blurPasses, uint8_t shadowAlpha)
{
    if (blurPasses < 1) blurPasses = 1;
    uint8_t passAlpha = shadowAlpha / blurPasses;
    for (int16_t i = blurPasses; i >= 1; i--) {
        fillCircleAlpha(cx + offsetX, cy + offsetY, radius + i * 2, 0x0000, passAlpha);
    }
}

// ============================================================================
// 9. SPRITE ENGINE (FLIP, SCALE, ROTATE, SHADOW)
// ============================================================================

void RGBGameEngine::drawSprite(const uint16_t *pixels, int16_t w, int16_t h, int16_t x, int16_t y, uint16_t colorKey, bool flipX, bool flipY)
{
    if (!pixels || !frameBuffer) return;
    for (int16_t sy = 0; sy < h; sy++) {
        int16_t dy = y + sy;
        if (dy < 0 || dy >= display.height) continue;
        int16_t srcY = flipY ? (h - 1 - sy) : sy;
        const uint16_t *srcRow = &pixels[srcY * w];
        uint16_t *dstRow = &frameBuffer[dy * display.width];

        for (int16_t sx = 0; sx < w; sx++) {
            int16_t dx = x + sx;
            if (dx < 0 || dx >= display.width) continue;
            int16_t srcX = flipX ? (w - 1 - sx) : sx;
            uint16_t c = srcRow[srcX];
            if (c != colorKey) {
                dstRow[dx] = c;
            }
        }
    }
}

void RGBGameEngine::drawSpriteAlpha(const uint16_t *pixels, int16_t w, int16_t h, int16_t x, int16_t y, uint8_t alpha, uint16_t colorKey, bool flipX)
{
    if (!pixels || !frameBuffer || alpha == 0) return;
    for (int16_t sy = 0; sy < h; sy++) {
        int16_t dy = y + sy;
        if (dy < 0 || dy >= display.height) continue;
        const uint16_t *srcRow = &pixels[sy * w];
        uint16_t *dstRow = &frameBuffer[dy * display.width];

        for (int16_t sx = 0; sx < w; sx++) {
            int16_t dx = x + sx;
            if (dx < 0 || dx >= display.width) continue;
            int16_t srcX = flipX ? (w - 1 - sx) : sx;
            uint16_t c = srcRow[srcX];
            if (c != colorKey) {
                dstRow[dx] = Color::blend565(c, dstRow[dx], alpha);
            }
        }
    }
}

void RGBGameEngine::drawSpriteTinted(const uint16_t *pixels, int16_t w, int16_t h, int16_t x, int16_t y, uint16_t tintColor, uint8_t alpha, uint16_t colorKey)
{
    if (!pixels || !frameBuffer || alpha == 0) return;
    for (int16_t sy = 0; sy < h; sy++) {
        int16_t dy = y + sy;
        if (dy < 0 || dy >= display.height) continue;
        const uint16_t *srcRow = &pixels[sy * w];
        uint16_t *dstRow = &frameBuffer[dy * display.width];

        for (int16_t sx = 0; sx < w; sx++) {
            int16_t dx = x + sx;
            if (dx < 0 || dx >= display.width) continue;
            uint16_t c = srcRow[sx];
            if (c != colorKey) {
                uint16_t tinted = Color::blend565(tintColor, c, 150);
                dstRow[dx] = Color::blend565(tinted, dstRow[dx], alpha);
            }
        }
    }
}

void RGBGameEngine::drawSpriteScaled(const uint16_t *pixels, int16_t srcW, int16_t srcH, int16_t dstX, int16_t dstY, int16_t dstW, int16_t dstH, uint16_t colorKey, bool flipX)
{
    if (!pixels || dstW <= 0 || dstH <= 0) return;
    for (int16_t y = 0; y < dstH; y++) {
        int16_t screenY = dstY + y;
        if (screenY < 0 || screenY >= display.height) continue;
        int16_t sy = (y * srcH) / dstH;
        const uint16_t *srcRow = &pixels[sy * srcW];
        uint16_t *dstRow = &frameBuffer[screenY * display.width];

        for (int16_t x = 0; x < dstW; x++) {
            int16_t screenX = dstX + x;
            if (screenX < 0 || screenX >= display.width) continue;
            int16_t sx = (x * srcW) / dstW;
            if (flipX) sx = srcW - 1 - sx;
            uint16_t c = srcRow[sx];
            if (c != colorKey) {
                dstRow[screenX] = c;
            }
        }
    }
}

void RGBGameEngine::drawSpriteRotated(const uint16_t *pixels, int16_t w, int16_t h, int16_t centerX, int16_t centerY, float angleDeg, uint16_t colorKey)
{
    if (!pixels) return;
    float rad = -angleDeg * 0.0174532925f;
    int32_t cosFp = (int32_t)(cosf(rad) * 256.0f);
    int32_t sinFp = (int32_t)(sinf(rad) * 256.0f);

    int16_t maxRadius = (int16_t)(sqrtf((float)(w * w + h * h)) * 0.5f) + 1;
    int16_t hw = w / 2, hh = h / 2;

    for (int16_t dy = -maxRadius; dy <= maxRadius; dy++) {
        int16_t sy = centerY + dy;
        if (sy < 0 || sy >= display.height) continue;
        uint16_t *dstRow = &frameBuffer[sy * display.width];

        for (int16_t dx = -maxRadius; dx <= maxRadius; dx++) {
            int16_t sx = centerX + dx;
            if (sx < 0 || sx >= display.width) continue;

            int16_t srcX = (int16_t)(((dx * cosFp - dy * sinFp) >> 8) + hw);
            int16_t srcY = (int16_t)(((dx * sinFp + dy * cosFp) >> 8) + hh);

            if (srcX >= 0 && srcX < w && srcY >= 0 && srcY < h) {
                uint16_t c = pixels[srcY * w + srcX];
                if (c != colorKey) dstRow[sx] = c;
            }
        }
    }
}

void RGBGameEngine::drawSpriteShadow(const uint16_t *pixels, int16_t w, int16_t h, int16_t footX, int16_t footY, float skewX, float scaleY, uint8_t shadowAlpha, uint16_t colorKey)
{
    if (!pixels || scaleY <= 0.01f) return;
    int16_t shadowH = (int16_t)(h * scaleY);
    for (int16_t sy = 0; sy < shadowH; sy++) {
        int16_t srcY = (sy * h) / shadowH;
        int16_t screenY = footY - shadowH + sy;
        if (screenY < 0 || screenY >= display.height) continue;

        int16_t shiftX = (int16_t)((shadowH - sy) * skewX);
        const uint16_t *srcRow = &pixels[srcY * w];
        uint16_t *dstRow = &frameBuffer[screenY * display.width];

        for (int16_t sx = 0; sx < w; sx++) {
            if (srcRow[sx] != colorKey) {
                int16_t screenX = footX + sx + shiftX;
                if (screenX >= 0 && screenX < display.width) {
                    dstRow[screenX] = Color::scaleBrightness565(dstRow[screenX], 255 - shadowAlpha);
                }
            }
        }
    }
}

// ============================================================================
// 10. BUILT-IN FONT & TEXT RENDERER
// ============================================================================

void RGBGameEngine::drawChar(int16_t x, int16_t y, char c, uint16_t color, uint8_t scale)
{
    if (c < 32 || c > 126 || scale == 0) return;
    const uint8_t *glyph = FONT5X7[c - 32];

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = pgm_read_byte(&glyph[col]);
        for (uint8_t row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                if (scale == 1) {
                    drawPixel(x + col, y + row, color);
                } else {
                    fillRect(x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

void RGBGameEngine::drawText(int16_t x, int16_t y, const char *str, uint16_t color, uint8_t scale)
{
    if (!str) return;
    int16_t curX = x;
    while (*str) {
        if (*str == '\n') {
            y += 9 * scale;
            curX = x;
        } else {
            drawChar(curX, y, *str, color, scale);
            curX += 6 * scale;
        }
        str++;
    }
}

void RGBGameEngine::drawTextShadow(int16_t x, int16_t y, const char *str, uint16_t textColor, uint16_t shadowColor, uint8_t scale, int16_t shadowOffset)
{
    drawText(x + shadowOffset, y + shadowOffset, str, shadowColor, scale);
    drawText(x, y, str, textColor, scale);
}

void RGBGameEngine::drawTextOutline(int16_t x, int16_t y, const char *str, uint16_t textColor, uint16_t outlineColor, uint8_t scale)
{
    drawText(x - 1, y,     str, outlineColor, scale);
    drawText(x + 1, y,     str, outlineColor, scale);
    drawText(x,     y - 1, str, outlineColor, scale);
    drawText(x,     y + 1, str, outlineColor, scale);
    drawText(x,     y,     str, textColor,    scale);
}

// ============================================================================
// 11. SCREEN POST-PROCESSING & CAMERA FILTERS
// ============================================================================

void RGBGameEngine::applyScreenTint(uint16_t tint565, uint8_t alpha)
{
    fillRectAlpha(0, 0, display.width, display.height, tint565, alpha);
}

void RGBGameEngine::applyRedGameOverFilter()
{
    const int total = display.width * display.height;
    for (int i = 0; i < total; i++) {
        frameBuffer[i] = (frameBuffer[i] & 0xF800) | 0x8000;
    }
}

void RGBGameEngine::applyCRTScanlines(uint8_t darkness, int16_t spacing)
{
    if (spacing < 2) spacing = 2;
    uint8_t keep = 255 - darkness;
    for (int16_t y = 0; y < display.height; y += spacing) {
        uint16_t *row = &frameBuffer[y * display.width];
        for (int16_t x = 0; x < display.width; x++) {
            row[x] = Color::scaleBrightness565(row[x], keep);
        }
    }
}

void RGBGameEngine::applyVignette(int16_t borderThickness, uint8_t maxDarkness)
{
    if (borderThickness <= 0) return;
    for (int16_t i = 0; i < borderThickness; i++) {
        uint8_t alpha = (uint8_t)(((int32_t)(borderThickness - i) * maxDarkness) / borderThickness);
        drawFastHLineAlpha(i, i, display.width - 2 * i, 0x0000, alpha);
        drawFastHLineAlpha(i, display.height - 1 - i, display.width - 2 * i, 0x0000, alpha);
        for (int16_t y = i + 1; y < display.height - 1 - i; y++) {
            drawPixelAlpha(i, y, 0x0000, alpha);
            drawPixelAlpha(display.width - 1 - i, y, 0x0000, alpha);
        }
    }
}

void RGBGameEngine::drawShockwaveRing(int16_t cx, int16_t cy, int16_t radius, int16_t bandWidth, float strength)
{
    if (radius <= 5 || bandWidth <= 2) return;
    int16_t rInner = max((int16_t)1, (int16_t)(radius - bandWidth));
    int16_t rOuter = radius + bandWidth;

    for (float a = 0.0f; a < 6.2831853f; a += 0.015f) {
        float ca = cosf(a), sa = sinf(a);
        int16_t sx = cx + (int16_t)(ca * (radius - strength));
        int16_t sy = cy + (int16_t)(sa * (radius - strength));
        int16_t dx = cx + (int16_t)(ca * radius);
        int16_t dy = cy + (int16_t)(sa * radius);
        uint16_t c = getPixel(sx, sy);
        drawPixel(dx, dy, Color::add565(c, Colors565::Cyan, 90));
    }
    drawCircle(cx, cy, rOuter, Colors565::ElectricBlue);
    drawCircle(cx, cy, rInner, Colors565::White);
}

void RGBGameEngine::drawIrisWipe(int16_t focusX, int16_t focusY, int16_t openRadius, uint16_t maskColor)
{
    if (openRadius <= 0) {
        clear(maskColor);
        return;
    }
    int32_t r2 = (int32_t)openRadius * openRadius;
    for (int16_t y = 0; y < display.height; y++) {
        int32_t dy = y - focusY;
        int32_t dy2 = dy * dy;
        if (dy2 >= r2) {
            drawFastHLine(0, y, display.width, maskColor);
        } else {
            int16_t span = (int16_t)sqrtf((float)(r2 - dy2));
            int16_t leftEnd = focusX - span;
            int16_t rightStart = focusX + span;
            if (leftEnd > 0) drawFastHLine(0, y, leftEnd, maskColor);
            if (rightStart < display.width) drawFastHLine(rightStart, y, display.width - rightStart, maskColor);
        }
    }
}

bool RGBGameEngine::playVideoFrame(const uint8_t *data)
{
    if (data == nullptr || frameBuffer == nullptr) return false;
    memcpy(frameBuffer, data, 800 * 480 * sizeof(uint16_t));
    show();
    return true;
}

void RGBGameEngine::show()
{
    if (lcd == nullptr || frameBuffer == nullptr) return;

    refreshFinished = false;
    lcd->drawBitmap(0, 0, 800, 480, (uint8_t *)frameBuffer);

    while (!refreshFinished) {
        vTaskDelay(1);
    }

    if (frameBuffer == frontBuffer) {
        frameBuffer = backBuffer;
    } else {
        frameBuffer = frontBuffer;
    }
}

void RGBGameEngine::onRefreshFinish()
{
    refreshFinished = true;
    swapPending = false;
}

void RGBGameEngine::refreshCallback()
{
    if (instance != nullptr) {
        instance->onRefreshFinish();
    }
}
