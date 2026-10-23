#ifndef COLOR_H
#define COLOR_H

#include <Arduino.h>
#include "PixelFormat.h"

// ============================================================================
// Compile-Time RGB888 to RGB565 Macro
// ============================================================================
#define RGB565_CONST(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)))

// ============================================================================
// Massive Precomputed RGB565 Color Library (140+ Colors, Zero CPU Cost)
// ============================================================================
namespace Colors565
{
    // --- Grayscale & Neutrals ---
    static constexpr uint16_t Black            = RGB565_CONST(0, 0, 0);
    static constexpr uint16_t Obsidian         = RGB565_CONST(12, 12, 18);
    static constexpr uint16_t Charcoal         = RGB565_CONST(34, 38, 42);
    static constexpr uint16_t Gunmetal         = RGB565_CONST(42, 52, 57);
    static constexpr uint16_t DarkSlateGray    = RGB565_CONST(47, 79, 79);
    static constexpr uint16_t DarkGray         = RGB565_CONST(64, 64, 64);
    static constexpr uint16_t DimGray          = RGB565_CONST(105, 105, 105);
    static constexpr uint16_t SlateGray        = RGB565_CONST(112, 128, 144);
    static constexpr uint16_t Gray             = RGB565_CONST(128, 128, 128);
    static constexpr uint16_t LightSlateGray   = RGB565_CONST(119, 136, 153);
    static constexpr uint16_t Silver           = RGB565_CONST(192, 192, 192);
    static constexpr uint16_t LightGray        = RGB565_CONST(211, 211, 211);
    static constexpr uint16_t Gainsboro        = RGB565_CONST(220, 220, 220);
    static constexpr uint16_t WhiteSmoke       = RGB565_CONST(245, 245, 245);
    static constexpr uint16_t Ivory            = RGB565_CONST(255, 255, 240);
    static constexpr uint16_t Snow             = RGB565_CONST(255, 250, 250);
    static constexpr uint16_t White            = RGB565_CONST(255, 255, 255);

    // --- Reds, Crimsons & Pinks ---
    static constexpr uint16_t Maroon           = RGB565_CONST(128, 0, 0);
    static constexpr uint16_t DarkRed          = RGB565_CONST(139, 0, 0);
    static constexpr uint16_t Burgundy         = RGB565_CONST(128, 0, 32);
    static constexpr uint16_t FireBrick        = RGB565_CONST(178, 34, 34);
    static constexpr uint16_t Crimson          = RGB565_CONST(220, 20, 60);
    static constexpr uint16_t Ruby             = RGB565_CONST(224, 17, 95);
    static constexpr uint16_t Scarlet          = RGB565_CONST(255, 36, 0);
    static constexpr uint16_t Red              = RGB565_CONST(255, 0, 0);
    static constexpr uint16_t NeonRed          = RGB565_CONST(255, 16, 48);
    static constexpr uint16_t Tomato           = RGB565_CONST(255, 99, 71);
    static constexpr uint16_t Coral            = RGB565_CONST(255, 127, 80);
    static constexpr uint16_t IndianRed        = RGB565_CONST(205, 92, 92);
    static constexpr uint16_t LightCoral       = RGB565_CONST(240, 128, 128);
    static constexpr uint16_t Salmon           = RGB565_CONST(250, 128, 114);
    static constexpr uint16_t LightSalmon      = RGB565_CONST(255, 160, 122);
    static constexpr uint16_t DeepPink         = RGB565_CONST(255, 20, 147);
    static constexpr uint16_t NeonPink         = RGB565_CONST(255, 16, 240);
    static constexpr uint16_t HotPink          = RGB565_CONST(255, 105, 180);
    static constexpr uint16_t Pink             = RGB565_CONST(255, 192, 203);
    static constexpr uint16_t LightPink        = RGB565_CONST(255, 182, 193);
    static constexpr uint16_t SakuraPink       = RGB565_CONST(255, 183, 197);
    static constexpr uint16_t PaleVioletRed    = RGB565_CONST(219, 112, 147);

    // --- Oranges, Ambers & Golds ---
    static constexpr uint16_t OrangeRed        = RGB565_CONST(255, 69, 0);
    static constexpr uint16_t DarkOrange       = RGB565_CONST(255, 140, 0);
    static constexpr uint16_t Tangerine        = RGB565_CONST(242, 133, 0);
    static constexpr uint16_t Orange           = RGB565_CONST(255, 165, 0);
    static constexpr uint16_t NeonOrange       = RGB565_CONST(255, 95, 31);
    static constexpr uint16_t Amber            = RGB565_CONST(255, 191, 0);
    static constexpr uint16_t Gold             = RGB565_CONST(255, 215, 0);
    static constexpr uint16_t GoldenRod        = RGB565_CONST(218, 165, 32);
    static constexpr uint16_t DarkGoldenRod    = RGB565_CONST(184, 134, 11);
    static constexpr uint16_t PeachPuff        = RGB565_CONST(255, 218, 185);
    static constexpr uint16_t Moccasin         = RGB565_CONST(255, 228, 181);
    static constexpr uint16_t PapayaWhip       = RGB565_CONST(255, 239, 213);

    // --- Yellows & Creams ---
    static constexpr uint16_t Yellow           = RGB565_CONST(255, 255, 0);
    static constexpr uint16_t ElectricYellow   = RGB565_CONST(255, 255, 51);
    static constexpr uint16_t LemonChiffon     = RGB565_CONST(255, 250, 205);
    static constexpr uint16_t LightYellow      = RGB565_CONST(255, 255, 224);
    static constexpr uint16_t Khaki            = RGB565_CONST(240, 230, 140);
    static constexpr uint16_t DarkKhaki        = RGB565_CONST(189, 183, 107);

    // --- Greens, Emeralds & Limes ---
    static constexpr uint16_t DarkGreen        = RGB565_CONST(0, 100, 0);
    static constexpr uint16_t ForestGreen      = RGB565_CONST(34, 139, 34);
    static constexpr uint16_t Green            = RGB565_CONST(0, 128, 0);
    static constexpr uint16_t PureGreen        = RGB565_CONST(0, 255, 0);
    static constexpr uint16_t NeonGreen        = RGB565_CONST(57, 255, 20);
    static constexpr uint16_t Lime             = RGB565_CONST(50, 205, 50);
    static constexpr uint16_t Chartreuse       = RGB565_CONST(127, 255, 0);
    static constexpr uint16_t LawnGreen        = RGB565_CONST(124, 252, 0);
    static constexpr uint16_t GreenYellow      = RGB565_CONST(173, 255, 47);
    static constexpr uint16_t YellowGreen      = RGB565_CONST(154, 205, 50);
    static constexpr uint16_t OliveDrab        = RGB565_CONST(107, 142, 35);
    static constexpr uint16_t Olive            = RGB565_CONST(128, 128, 0);
    static constexpr uint16_t DarkOliveGreen   = RGB565_CONST(85, 107, 47);
    static constexpr uint16_t SeaGreen         = RGB565_CONST(46, 139, 87);
    static constexpr uint16_t MediumSeaGreen   = RGB565_CONST(60, 179, 113);
    static constexpr uint16_t Emerald          = RGB565_CONST(80, 200, 120);
    static constexpr uint16_t Jade             = RGB565_CONST(0, 168, 107);
    static constexpr uint16_t SpringGreen      = RGB565_CONST(0, 255, 127);
    static constexpr uint16_t MediumSpringGreen= RGB565_CONST(0, 250, 154);
    static constexpr uint16_t Mint             = RGB565_CONST(152, 255, 152);
    static constexpr uint16_t LightGreen       = RGB565_CONST(144, 238, 144);
    static constexpr uint16_t PaleGreen        = RGB565_CONST(152, 251, 152);

    // --- Cyans, Teals & Aquas ---
    static constexpr uint16_t Teal             = RGB565_CONST(0, 128, 128);
    static constexpr uint16_t DarkCyan         = RGB565_CONST(0, 139, 139);
    static constexpr uint16_t LightSeaGreen    = RGB565_CONST(32, 178, 170);
    static constexpr uint16_t CadetBlue        = RGB565_CONST(95, 158, 160);
    static constexpr uint16_t DarkTurquoise    = RGB565_CONST(0, 206, 209);
    static constexpr uint16_t MediumTurquoise  = RGB565_CONST(72, 209, 204);
    static constexpr uint16_t Turquoise        = RGB565_CONST(64, 224, 208);
    static constexpr uint16_t Aquamarine       = RGB565_CONST(127, 255, 212);
    static constexpr uint16_t Cyan             = RGB565_CONST(0, 255, 255);
    static constexpr uint16_t Aqua             = RGB565_CONST(0, 255, 255);
    static constexpr uint16_t NeonCyan         = RGB565_CONST(0, 245, 255);
    static constexpr uint16_t PaleTurquoise    = RGB565_CONST(175, 238, 238);
    static constexpr uint16_t LightCyan        = RGB565_CONST(224, 255, 255);

    // --- Blues, Cobalts & Navys ---
    static constexpr uint16_t MidnightBlue     = RGB565_CONST(25, 25, 112);
    static constexpr uint16_t Navy             = RGB565_CONST(0, 0, 128);
    static constexpr uint16_t DarkBlue         = RGB565_CONST(0, 0, 139);
    static constexpr uint16_t MediumBlue       = RGB565_CONST(0, 0, 205);
    static constexpr uint16_t Blue             = RGB565_CONST(0, 0, 255);
    static constexpr uint16_t Cobalt           = RGB565_CONST(0, 71, 171);
    static constexpr uint16_t RoyalBlue        = RGB565_CONST(65, 105, 225);
    static constexpr uint16_t SteelBlue        = RGB565_CONST(70, 130, 180);
    static constexpr uint16_t DodgerBlue       = RGB565_CONST(30, 144, 255);
    static constexpr uint16_t ElectricBlue     = RGB565_CONST(125, 249, 255);
    static constexpr uint16_t DeepSkyBlue      = RGB565_CONST(0, 191, 255);
    static constexpr uint16_t CornflowerBlue   = RGB565_CONST(100, 149, 237);
    static constexpr uint16_t SkyBlue          = RGB565_CONST(135, 206, 235);
    static constexpr uint16_t LightSkyBlue     = RGB565_CONST(135, 206, 250);
    static constexpr uint16_t LightSteelBlue   = RGB565_CONST(176, 196, 222);
    static constexpr uint16_t LightBlue        = RGB565_CONST(173, 216, 230);
    static constexpr uint16_t PowderBlue       = RGB565_CONST(176, 224, 230);
    static constexpr uint16_t Azure            = RGB565_CONST(240, 255, 255);
    static constexpr uint16_t AliceBlue        = RGB565_CONST(240, 248, 255);

    // --- Purples, Violets & Magentas ---
    static constexpr uint16_t Indigo           = RGB565_CONST(75, 0, 130);
    static constexpr uint16_t Purple           = RGB565_CONST(128, 0, 128);
    static constexpr uint16_t DarkMagenta      = RGB565_CONST(139, 0, 139);
    static constexpr uint16_t DarkViolet       = RGB565_CONST(148, 0, 211);
    static constexpr uint16_t DarkSlateBlue    = RGB565_CONST(72, 61, 139);
    static constexpr uint16_t BlueViolet       = RGB565_CONST(138, 43, 226);
    static constexpr uint16_t Amethyst         = RGB565_CONST(153, 102, 204);
    static constexpr uint16_t DarkOrchid       = RGB565_CONST(153, 50, 204);
    static constexpr uint16_t MediumPurple     = RGB565_CONST(147, 112, 219);
    static constexpr uint16_t SlateBlue        = RGB565_CONST(106, 90, 205);
    static constexpr uint16_t MediumSlateBlue  = RGB565_CONST(123, 104, 238);
    static constexpr uint16_t MediumOrchid     = RGB565_CONST(186, 85, 211);
    static constexpr uint16_t Magenta          = RGB565_CONST(255, 0, 255);
    static constexpr uint16_t Fuchsia          = RGB565_CONST(255, 0, 255);
    static constexpr uint16_t NeonPurple       = RGB565_CONST(188, 19, 254);
    static constexpr uint16_t Orchid           = RGB565_CONST(218, 112, 214);
    static constexpr uint16_t Violet           = RGB565_CONST(238, 130, 238);
    static constexpr uint16_t Plum             = RGB565_CONST(221, 160, 221);
    static constexpr uint16_t Thistle          = RGB565_CONST(216, 191, 216);
    static constexpr uint16_t Lavender         = RGB565_CONST(230, 230, 250);

    // --- Browns, Earth & Metals ---
    static constexpr uint16_t Espresso         = RGB565_CONST(59, 47, 47);
    static constexpr uint16_t SaddleBrown      = RGB565_CONST(139, 69, 19);
    static constexpr uint16_t Brown            = RGB565_CONST(139, 69, 19);
    static constexpr uint16_t Sienna           = RGB565_CONST(160, 82, 45);
    static constexpr uint16_t Chocolate        = RGB565_CONST(210, 105, 30);
    static constexpr uint16_t Copper           = RGB565_CONST(184, 115, 51);
    static constexpr uint16_t Bronze           = RGB565_CONST(205, 127, 50);
    static constexpr uint16_t Peru             = RGB565_CONST(205, 133, 63);
    static constexpr uint16_t SandyBrown       = RGB565_CONST(244, 164, 96);
    static constexpr uint16_t BurlyWood        = RGB565_CONST(222, 184, 135);
    static constexpr uint16_t Tan              = RGB565_CONST(210, 180, 140);
    static constexpr uint16_t RosyBrown        = RGB565_CONST(188, 143, 143);
    static constexpr uint16_t Wheat            = RGB565_CONST(245, 222, 179);
    static constexpr uint16_t Beige            = RGB565_CONST(245, 245, 220);
}

// ============================================================================
// Color Class + Color Physics & Bitwise RGB565 Math Engine
// ============================================================================
class Color
{
public:
    uint8_t r;
    uint8_t g;
    uint8_t b;

    Color();
    Color(uint8_t red, uint8_t green, uint8_t blue);

    uint16_t rgb565() const;
    static Color fromRGB565(uint16_t c);

    // --- Classic Named Colors ---
    static Color Black();
    static Color White();
    static Color Red();
    static Color Green();
    static Color Blue();
    static Color Yellow();
    static Color Cyan();
    static Color Magenta();
    static Color Orange();
    static Color Purple();
    static Color Pink();
    static Color Brown();
    static Color Gray();
    static Color LightGray();
    static Color DarkGray();
    static Color Navy();
    static Color Lime();
    static Color Olive();
    static Color Teal();
    static Color Silver();
    static Color Gold();

    // --- Color Space & Physical Light Spectrum Generators ---
    // HSV: h in [0..360], s in [0..255], v in [0..255]
    static Color fromHSV(uint16_t h, uint8_t s, uint8_t v);
    static uint16_t hsv565(uint16_t h, uint8_t s, uint8_t v);

    // HSL: h in [0..360], s in [0..255], l in [0..255]
    static Color fromHSL(uint16_t h, uint8_t s, uint8_t l);

    // Blackbody Radiation Physics: tempKelvin in [1000K..12000K]
    // 1000K = deep red ember, 3000K = warm bulb, 5500K = noon sun, 9000K = blue plasma
    static Color fromKelvin(uint16_t tempKelvin);
    static uint16_t kelvin565(uint16_t tempKelvin);

    // Visible Light Wavelength Physics: wavelengthNm in [380nm..750nm]
    static Color fromWavelength(float wavelengthNm);
    static uint16_t wavelength565(float wavelengthNm);

    // --- Exact Per-Channel Hardware Alpha Blend for RGB565 ---
    // Blends foreground (fg) over background (bg) with alpha in [0..255] (0 = transparent bg, 255 = opaque fg)
    static inline uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t alpha)
    {
        if (alpha == 0)   return bg;
        if (alpha == 255) return fg;
        uint32_t a = ((uint32_t)alpha + 4U) >> 3; // 0..32
        uint32_t invA = 32U - a;

        uint32_t r = ((((fg >> 11) & 0x1FU) * a) + (((bg >> 11) & 0x1FU) * invA)) >> 5;
        uint32_t g = ((((fg >> 5)  & 0x3FU) * a) + (((bg >> 5)  & 0x3FU) * invA)) >> 5;
        uint32_t b = ((( fg        & 0x1FU) * a) + (( bg        & 0x1FU) * invA)) >> 5;
        return (uint16_t)((r << 11) | (g << 5) | b);
    }

    // Linear interpolation between two RGB565 colors (t in 0..255)
    static inline uint16_t lerp565(uint16_t c1, uint16_t c2, uint8_t t)
    {
        return blend565(c2, c1, t);
    }

    // Scale an RGB565 color brightness by factor (0 = black, 255 = original color)
    static inline uint16_t scaleBrightness565(uint16_t c, uint8_t brightness)
    {
        if (brightness == 0)   return 0x0000;
        if (brightness == 255) return c;
        uint32_t a = ((uint32_t)brightness + 4U) >> 3; // 0..32
        uint32_t r = (((c >> 11) & 0x1FU) * a) >> 5;
        uint32_t g = (((c >> 5)  & 0x3FU) * a) >> 5;
        uint32_t b = (( c        & 0x1FU) * a) >> 5;
        return (uint16_t)((r << 11) | (g << 5) | b);
    }

    // Additive saturating blend (great for light emitters, lasers, fire, glow)
    static uint16_t add565(uint16_t c1, uint16_t c2, uint8_t intensity = 255);

    // Multiply blend (shadows, stained glass, color filters)
    static uint16_t multiply565(uint16_t c1, uint16_t c2);

    // Screen blend (soft bloom / highlight)
    static uint16_t screen565(uint16_t c1, uint16_t c2);

    // Overlay blend (high-contrast Multiply + Screen hybrid)
    static uint16_t overlay565(uint16_t src, uint16_t dst);

    // Soft-light blend (warm studio contrast)
    static uint16_t softLight565(uint16_t src, uint16_t dst);

    // Difference blend (|src - dst| X-ray / solarize)
    static uint16_t difference565(uint16_t src, uint16_t dst);

    // Exclusion blend (src + dst - 2*src*dst)
    static uint16_t exclusion565(uint16_t src, uint16_t dst);

    // Color Dodge blend (bright highlight blowout)
    static uint16_t colorDodge565(uint16_t src, uint16_t dst);

    // Color Burn blend (deep cinema shadow burn)
    static uint16_t colorBurn565(uint16_t src, uint16_t dst);

    // Subtractive blend (energy drain)
    static uint16_t subtract565(uint16_t dst, uint16_t src, uint8_t intensity = 255);

    // Universal blend mode dispatcher (17 studio modes)
    static uint16_t applyBlendMode565(uint16_t src, uint16_t dst, uint8_t alpha, BlendMode mode);

    // Fast perceptual grayscale, sepia, and invert in RGB565
    static uint16_t grayscale565(uint16_t c);
    static uint16_t sepia565(uint16_t c);
    static inline uint16_t invert565(uint16_t c) { return ~c; }

    // 4x4 Bayer Ordered Dithering: eliminates color banding in RGB565 gradients!
    static uint16_t ditherRGBto565(uint8_t r, uint8_t g, uint8_t b, int16_t x, int16_t y);
};

#endif
