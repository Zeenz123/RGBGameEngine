#include "Color.h"
#include <math.h>

// 4x4 Bayer Matrix for ordered dithering (values 0..15)
static const uint8_t BAYER_4X4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
};

Color::Color() : r(0), g(0), b(0) {}

Color::Color(uint8_t red, uint8_t green, uint8_t blue)
    : r(red), g(green), b(blue) {}

uint16_t Color::rgb565() const
{
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}

Color Color::fromRGB565(uint16_t c)
{
    uint8_t r5 = (c >> 11) & 0x1F;
    uint8_t g6 = (c >> 5)  & 0x3F;
    uint8_t b5 =  c        & 0x1F;
    return Color(
        (r5 << 3) | (r5 >> 2),
        (g6 << 2) | (g6 >> 4),
        (b5 << 3) | (b5 >> 2)
    );
}

Color Color::Black()     { return Color(0, 0, 0); }
Color Color::White()     { return Color(255, 255, 255); }
Color Color::Red()       { return Color(255, 0, 0); }
Color Color::Green()     { return Color(0, 255, 0); }
Color Color::Blue()      { return Color(0, 0, 255); }
Color Color::Yellow()    { return Color(255, 255, 0); }
Color Color::Cyan()      { return Color(0, 255, 255); }
Color Color::Magenta()   { return Color(255, 0, 255); }
Color Color::Orange()    { return Color(255, 165, 0); }
Color Color::Purple()    { return Color(128, 0, 128); }
Color Color::Pink()      { return Color(255, 105, 180); }
Color Color::Brown()     { return Color(139, 69, 19); }
Color Color::Gray()      { return Color(128, 128, 128); }
Color Color::LightGray() { return Color(211, 211, 211); }
Color Color::DarkGray()  { return Color(64, 64, 64); }
Color Color::Navy()      { return Color(0, 0, 128); }
Color Color::Lime()      { return Color(50, 205, 50); }
Color Color::Olive()     { return Color(128, 128, 0); }
Color Color::Teal()      { return Color(0, 128, 128); }
Color Color::Silver()    { return Color(192, 192, 192); }
Color Color::Gold()      { return Color(255, 215, 0); }

// ============================================================================
// Fast Integer HSV -> RGB Conversion
// ============================================================================
Color Color::fromHSV(uint16_t h, uint8_t s, uint8_t v)
{
    if (s == 0) return Color(v, v, v);

    h = h % 360;
    uint8_t region = h / 60;
    uint16_t remainder = (h % 60) * 255 / 60;

    uint8_t p = (uint16_t)(v * (255 - s)) >> 8;
    uint8_t q = (uint16_t)(v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (uint16_t)(v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:  return Color(v, t, p);
        case 1:  return Color(q, v, p);
        case 2:  return Color(p, v, t);
        case 3:  return Color(p, q, v);
        case 4:  return Color(t, p, v);
        default: return Color(v, p, q);
    }
}

uint16_t Color::hsv565(uint16_t h, uint8_t s, uint8_t v)
{
    return fromHSV(h, s, v).rgb565();
}

Color Color::fromHSL(uint16_t h, uint8_t s, uint8_t l)
{
    float hf = (float)(h % 360) / 60.0f;
    float sf = s / 255.0f;
    float lf = l / 255.0f;

    float c = (1.0f - fabsf(2.0f * lf - 1.0f)) * sf;
    float x = c * (1.0f - fabsf(fmodf(hf, 2.0f) - 1.0f));
    float m = lf - c * 0.5f;

    float rf = 0, gf = 0, bf = 0;
    if      (hf < 1.0f) { rf = c; gf = x; bf = 0; }
    else if (hf < 2.0f) { rf = x; gf = c; bf = 0; }
    else if (hf < 3.0f) { rf = 0; gf = c; bf = x; }
    else if (hf < 4.0f) { rf = 0; gf = x; bf = c; }
    else if (hf < 5.0f) { rf = x; gf = 0; bf = c; }
    else                { rf = c; gf = 0; bf = x; }

    return Color(
        (uint8_t)((rf + m) * 255.0f),
        (uint8_t)((gf + m) * 255.0f),
        (uint8_t)((bf + m) * 255.0f)
    );
}

// ============================================================================
// Blackbody Radiation Physics (Tanner Helland Planckian Locus Approximation)
// ============================================================================
Color Color::fromKelvin(uint16_t tempKelvin)
{
    if (tempKelvin < 1000)  tempKelvin = 1000;
    if (tempKelvin > 15000) tempKelvin = 15000;

    float temp = tempKelvin / 100.0f;
    float rf, gf, bf;

    // Red
    if (temp <= 66.0f) {
        rf = 255.0f;
    } else {
        rf = 329.698727446f * powf(temp - 60.0f, -0.1332047592f);
        if (rf < 0.0f) rf = 0.0f;
        if (rf > 255.0f) rf = 255.0f;
    }

    // Green
    if (temp <= 66.0f) {
        gf = 99.4708025861f * logf(temp) - 161.1195681661f;
    } else {
        gf = 288.1221695283f * powf(temp - 60.0f, -0.0755148492f);
    }
    if (gf < 0.0f) gf = 0.0f;
    if (gf > 255.0f) gf = 255.0f;

    // Blue
    if (temp >= 66.0f) {
        bf = 255.0f;
    } else if (temp <= 19.0f) {
        bf = 0.0f;
    } else {
        bf = 138.5177312231f * logf(temp - 10.0f) - 305.0447927307f;
        if (bf < 0.0f) bf = 0.0f;
        if (bf > 255.0f) bf = 255.0f;
    }

    return Color((uint8_t)rf, (uint8_t)gf, (uint8_t)bf);
}

uint16_t Color::kelvin565(uint16_t tempKelvin)
{
    return fromKelvin(tempKelvin).rgb565();
}

// ============================================================================
// Visible Spectrum Wavelength Physics (380nm Violet -> 750nm Deep Red)
// ============================================================================
Color Color::fromWavelength(float wl)
{
    float rf = 0.0f, gf = 0.0f, bf = 0.0f;

    if (wl >= 380.0f && wl < 440.0f) {
        rf = -(wl - 440.0f) / (440.0f - 380.0f);
        bf = 1.0f;
    } else if (wl >= 440.0f && wl < 490.0f) {
        gf = (wl - 440.0f) / (490.0f - 440.0f);
        bf = 1.0f;
    } else if (wl >= 490.0f && wl < 510.0f) {
        gf = 1.0f;
        bf = -(wl - 510.0f) / (510.0f - 490.0f);
    } else if (wl >= 510.0f && wl < 580.0f) {
        rf = (wl - 510.0f) / (580.0f - 510.0f);
        gf = 1.0f;
    } else if (wl >= 580.0f && wl < 645.0f) {
        rf = 1.0f;
        gf = -(wl - 645.0f) / (645.0f - 580.0f);
    } else if (wl >= 645.0f && wl <= 750.0f) {
        rf = 1.0f;
    }

    // Eye sensitivity attenuation near spectrum edges
    float factor = 1.0f;
    if (wl >= 380.0f && wl < 420.0f) {
        factor = 0.3f + 0.7f * (wl - 380.0f) / (420.0f - 380.0f);
    } else if (wl > 700.0f && wl <= 750.0f) {
        factor = 0.3f + 0.7f * (750.0f - wl) / (750.0f - 700.0f);
    }

    return Color(
        (uint8_t)(rf * factor * 255.0f),
        (uint8_t)(gf * factor * 255.0f),
        (uint8_t)(bf * factor * 255.0f)
    );
}

uint16_t Color::wavelength565(float wavelengthNm)
{
    return fromWavelength(wavelengthNm).rgb565();
}

// ============================================================================
// RGB565 Blend Modes & Filters
// ============================================================================
uint16_t Color::add565(uint16_t c1, uint16_t c2, uint8_t intensity)
{
    if (intensity < 255) {
        c2 = scaleBrightness565(c2, intensity);
    }
    uint16_t r = ((c1 >> 11) & 0x1F) + ((c2 >> 11) & 0x1F);
    uint16_t g = ((c1 >> 5)  & 0x3F) + ((c2 >> 5)  & 0x3F);
    uint16_t b = ( c1        & 0x1F) + ( c2        & 0x1F);

    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (b > 31) b = 31;

    return (r << 11) | (g << 5) | b;
}

uint16_t Color::multiply565(uint16_t c1, uint16_t c2)
{
    uint16_t r = (((c1 >> 11) & 0x1F) * ((c2 >> 11) & 0x1F)) / 31;
    uint16_t g = (((c1 >> 5)  & 0x3F) * ((c2 >> 5)  & 0x3F)) / 63;
    uint16_t b = (( c1        & 0x1F) * ( c2        & 0x1F)) / 31;
    return (r << 11) | (g << 5) | b;
}

uint16_t Color::screen565(uint16_t c1, uint16_t c2)
{
    uint16_t r1 = (c1 >> 11) & 0x1F, r2 = (c2 >> 11) & 0x1F;
    uint16_t g1 = (c1 >> 5)  & 0x3F, g2 = (c2 >> 5)  & 0x3F;
    uint16_t b1 =  c1        & 0x1F, b2 =  c2        & 0x1F;

    uint16_t r = 31 - (((31 - r1) * (31 - r2)) / 31);
    uint16_t g = 63 - (((63 - g1) * (63 - g2)) / 63);
    uint16_t b = 31 - (((31 - b1) * (31 - b2)) / 31);
    return (r << 11) | (g << 5) | b;
}

uint16_t Color::overlay565(uint16_t src, uint16_t dst)
{
    uint16_t sr = (src >> 11) & 0x1F, dr = (dst >> 11) & 0x1F;
    uint16_t sg = (src >> 5)  & 0x3F, dg = (dst >> 5)  & 0x3F;
    uint16_t sb =  src        & 0x1F, db =  dst        & 0x1F;

    uint16_t r = (dr < 16) ? ((2 * sr * dr) / 31) : (31 - (2 * (31 - sr) * (31 - dr)) / 31);
    uint16_t g = (dg < 32) ? ((2 * sg * dg) / 63) : (63 - (2 * (63 - sg) * (63 - dg)) / 63);
    uint16_t b = (db < 16) ? ((2 * sb * db) / 31) : (31 - (2 * (31 - sb) * (31 - db)) / 31);
    return (r << 11) | (g << 5) | b;
}

uint16_t Color::softLight565(uint16_t src, uint16_t dst)
{
    uint32_t sr = (src >> 11) & 0x1F, dr = (dst >> 11) & 0x1F;
    uint32_t sg = (src >> 5)  & 0x3F, dg = (dst >> 5)  & 0x3F;
    uint32_t sb =  src        & 0x1F, db =  dst        & 0x1F;

    // Pegtop Soft-Light formula in 5/6-bit integer space: ((max - 2*s)*d*d + 2*s*d*max) / max^2
    uint16_t r = (uint16_t)(((31U - dr) * ((sr * dr) / 31U) + dr * (31U - ((31U - sr) * (31U - dr)) / 31U)) / 31U);
    uint16_t g = (uint16_t)(((63U - dg) * ((sg * dg) / 63U) + dg * (63U - ((63U - sg) * (63U - dg)) / 63U)) / 63U);
    uint16_t b = (uint16_t)(((31U - db) * ((sb * db) / 31U) + db * (31U - ((31U - sb) * (31U - db)) / 31U)) / 31U);
    return (r << 11) | (g << 5) | b;
}

uint16_t Color::difference565(uint16_t src, uint16_t dst)
{
    int16_t sr = (src >> 11) & 0x1F, dr = (dst >> 11) & 0x1F;
    int16_t sg = (src >> 5)  & 0x3F, dg = (dst >> 5)  & 0x3F;
    int16_t sb =  src        & 0x1F, db =  dst        & 0x1F;

    uint16_t r = (uint16_t)abs(sr - dr);
    uint16_t g = (uint16_t)abs(sg - dg);
    uint16_t b = (uint16_t)abs(sb - db);
    return (r << 11) | (g << 5) | b;
}

uint16_t Color::exclusion565(uint16_t src, uint16_t dst)
{
    uint16_t sr = (src >> 11) & 0x1F, dr = (dst >> 11) & 0x1F;
    uint16_t sg = (src >> 5)  & 0x3F, dg = (dst >> 5)  & 0x3F;
    uint16_t sb =  src        & 0x1F, db =  dst        & 0x1F;

    uint16_t r = sr + dr - ((2 * sr * dr) / 31);
    uint16_t g = sg + dg - ((2 * sg * dg) / 63);
    uint16_t b = sb + db - ((2 * sb * db) / 31);
    return (r << 11) | (g << 5) | b;
}

uint16_t Color::colorDodge565(uint16_t src, uint16_t dst)
{
    uint16_t sr = (src >> 11) & 0x1F, dr = (dst >> 11) & 0x1F;
    uint16_t sg = (src >> 5)  & 0x3F, dg = (dst >> 5)  & 0x3F;
    uint16_t sb =  src        & 0x1F, db =  dst        & 0x1F;

    uint16_t r = (sr >= 31) ? 31 : min((uint16_t)31, (uint16_t)((dr * 31) / (31 - sr)));
    uint16_t g = (sg >= 63) ? 63 : min((uint16_t)63, (uint16_t)((dg * 63) / (63 - sg)));
    uint16_t b = (sb >= 31) ? 31 : min((uint16_t)31, (uint16_t)((db * 31) / (31 - sb)));
    return (r << 11) | (g << 5) | b;
}

uint16_t Color::colorBurn565(uint16_t src, uint16_t dst)
{
    uint16_t sr = (src >> 11) & 0x1F, dr = (dst >> 11) & 0x1F;
    uint16_t sg = (src >> 5)  & 0x3F, dg = (dst >> 5)  & 0x3F;
    uint16_t sb =  src        & 0x1F, db =  dst        & 0x1F;

    uint16_t r = (sr == 0) ? 0 : (31 - min((uint16_t)31, (uint16_t)(((31 - dr) * 31) / sr)));
    uint16_t g = (sg == 0) ? 0 : (63 - min((uint16_t)63, (uint16_t)(((63 - dg) * 63) / sg)));
    uint16_t b = (sb == 0) ? 0 : (31 - min((uint16_t)31, (uint16_t)(((31 - db) * 31) / sb)));
    return (r << 11) | (g << 5) | b;
}

uint16_t Color::subtract565(uint16_t dst, uint16_t src, uint8_t intensity)
{
    if (intensity < 255) {
        src = scaleBrightness565(src, intensity);
    }
    int16_t r = (int16_t)((dst >> 11) & 0x1F) - (int16_t)((src >> 11) & 0x1F);
    int16_t g = (int16_t)((dst >> 5)  & 0x3F) - (int16_t)((src >> 5)  & 0x3F);
    int16_t b = (int16_t)( dst        & 0x1F) - (int16_t)( src        & 0x1F);

    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return ((uint16_t)r << 11) | ((uint16_t)g << 5) | (uint16_t)b;
}

uint16_t Color::applyBlendMode565(uint16_t src, uint16_t dst, uint8_t alpha, BlendMode mode)
{
    switch (mode)
    {
        case BlendMode::SOLID:
            return src;
        case BlendMode::ALPHA:
            return blend565(src, dst, alpha);
        case BlendMode::ADDITIVE:
            return add565(dst, src, alpha);
        case BlendMode::SUBTRACTIVE:
            return subtract565(dst, src, alpha);
        case BlendMode::MULTIPLY:
            return blend565(multiply565(src, dst), dst, alpha);
        case BlendMode::SCREEN:
            return blend565(screen565(src, dst), dst, alpha);
        case BlendMode::OVERLAY:
            return blend565(overlay565(src, dst), dst, alpha);
        case BlendMode::SOFT_LIGHT:
            return blend565(softLight565(src, dst), dst, alpha);
        case BlendMode::HARD_LIGHT:
            return blend565(overlay565(dst, src), dst, alpha);
        case BlendMode::COLOR_DODGE:
            return blend565(colorDodge565(src, dst), dst, alpha);
        case BlendMode::COLOR_BURN:
            return blend565(colorBurn565(src, dst), dst, alpha);
        case BlendMode::DIFFERENCE:
            return blend565(difference565(src, dst), dst, alpha);
        case BlendMode::EXCLUSION:
            return blend565(exclusion565(src, dst), dst, alpha);
        case BlendMode::COLOR_TINT: {
            uint16_t mask = ((src & 0xF800) ? 0xF800 : 0) |
                            ((src & 0x07E0) ? 0x07E0 : 0) |
                            ((src & 0x001F) ? 0x001F : 0);
            uint16_t tinted = (dst & mask) | (src & 0x8410);
            return blend565(tinted, dst, alpha);
        }
        case BlendMode::LIGHTEN: {
            uint16_t r = max((src >> 11) & 0x1F, (dst >> 11) & 0x1F);
            uint16_t g = max((src >> 5)  & 0x3F, (dst >> 5)  & 0x3F);
            uint16_t b = max( src        & 0x1F,  dst        & 0x1F);
            return blend565((r << 11) | (g << 5) | b, dst, alpha);
        }
        case BlendMode::DARKEN: {
            uint16_t r = min((src >> 11) & 0x1F, (dst >> 11) & 0x1F);
            uint16_t g = min((src >> 5)  & 0x3F, (dst >> 5)  & 0x3F);
            uint16_t b = min( src        & 0x1F,  dst        & 0x1F);
            return blend565((r << 11) | (g << 5) | b, dst, alpha);
        }
        case BlendMode::CHECKER_STIPPLE:
            return (alpha >= 128) ? src : dst;
        default:
            return src;
    }
}

uint16_t Color::grayscale565(uint16_t c)
{
    uint16_t r8 = ((c >> 11) & 0x1F) << 3;
    uint16_t g8 = ((c >> 5)  & 0x3F) << 2;
    uint16_t b8 = ( c        & 0x1F) << 3;
    // ITU-R BT.601 luminance: Y = (77*R + 150*G + 29*B) >> 8
    uint8_t y = (uint8_t)((77 * r8 + 150 * g8 + 29 * b8) >> 8);
    return RGB565_CONST(y, y, y);
}

uint16_t Color::sepia565(uint16_t c)
{
    uint16_t r8 = ((c >> 11) & 0x1F) << 3;
    uint16_t g8 = ((c >> 5)  & 0x3F) << 2;
    uint16_t b8 = ( c        & 0x1F) << 3;

    uint16_t tr = (101 * r8 + 197 * g8 + 48 * b8) >> 8;
    uint16_t tg = ( 89 * r8 + 176 * g8 + 43 * b8) >> 8;
    uint16_t tb = ( 70 * r8 + 137 * g8 + 33 * b8) >> 8;

    if (tr > 255) tr = 255;
    if (tg > 255) tg = 255;
    if (tb > 255) tb = 255;
    return RGB565_CONST(tr, tg, tb);
}

uint16_t Color::ditherRGBto565(uint8_t r, uint8_t g, uint8_t b, int16_t x, int16_t y)
{
    uint8_t threshold = BAYER_4X4[y & 3][x & 3];
    // Add sub-bit Bayer threshold before truncating to 5-6-5 bits
    uint16_t dr = r + (threshold >> 1);
    uint16_t dg = g + (threshold >> 2);
    uint16_t db = b + (threshold >> 1);
    if (dr > 255) dr = 255;
    if (dg > 255) dg = 255;
    if (db > 255) db = 255;
    return RGB565_CONST(dr, dg, db);
}
