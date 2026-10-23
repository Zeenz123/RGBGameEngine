#include <Arduino.h>
#include <esp_io_expander.hpp>
#include "RGBGameEngine.h"

// ---------------- WAVESHARE ESP32-S3 LCD PINS ----------------
#define WS_CH422G_SCL 9
#define WS_CH422G_SDA 8
#define TP_RST 1
#define LCD_BL 2
#define LCD_RST 3
#define SD_CS 4
#define USB_SEL 5

esp_expander::CH422G *expander = nullptr;
RGBGameEngine engine;

// ---------------- SUBSYSTEM INSTANCES FOR TESTING ----------------
ShatterSystem2D   shatterLab;
SoftBodyCushion2D cushionLab;
VerletCloth2D     clothLab;
VerletRope2D      scarfLab;
WaterSurface2D    waterLab;
WeatherSystem2D   weatherLab;
RibbonTrail2D     swordTrail;
Spring2D          springLab;
RigidBody2D       ballA, ballB;
ParticleSystem2D  particleLab;

// ---------------- INTERACTIVE TEST STATE ----------------
static int   currentScene        = 1;     // 1..9
static bool  autoCycleScenes     = false; // Default to LOCKED so user can test freely ('0' toggles Auto-Cycle)
static uint32_t sceneStartTime   = 0;

// Universal Intensity Control (0.1f to 2.5f) - affects EVERY scene!
static float globalIntensity     = 1.2f;

// Live Interactive Cursor / Object Controller (Moved via W/A/S/D or Arrow keys in control.py)
static float cursorX             = 320.0f;
static float cursorY             = 240.0f;
static float velX                = 0.0f;
static float velY                = 0.0f;
static bool  manualControlActive = false;
static uint32_t lastInputMs      = 0;

// Sub-modes per scene (Cycled via 'M' and 'B' or 'SPACE')
static uint8_t blendLensModeIdx    = 0;     // Scene 3: 10 interactive transparency/blend lens modes!
static uint8_t shatterMatIdx       = 0;     // Scene 5: 5 physical materials
static uint8_t clothColorIdx       = 0;     // Scene 7: Silk fabric color presets
static uint8_t particleShapeIdx    = 0;     // Scene 9: 6 emitter shapes
static uint8_t particleBehaviorIdx = 0;     // Scene 9: 10 behavior patterns
static bool    clothTearMode       = false;
static bool    moveStickInScene4   = false;

// Scene 6 Physical Bouncing Cushion Ball State
static float   cushionBallX        = 400.0f;
static float   cushionBallY        = 115.0f;
static float   cushionBallVx       = 75.0f;
static float   cushionBallVy       = 0.0f;

bool initCH422G()
{
    expander = new esp_expander::CH422G(WS_CH422G_SCL, WS_CH422G_SDA, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS);
    if (!expander) return false;
    expander->init();
    expander->begin();
    expander->enableAllIO_Output();
    expander->digitalWrite(TP_RST, HIGH);
    expander->digitalWrite(LCD_BL, HIGH);
    expander->digitalWrite(LCD_RST, HIGH);
    expander->digitalWrite(USB_SEL, LOW);
    expander->digitalWrite(SD_CS, LOW);
    return true;
}

void configureParticlePreset(uint8_t shapeIdx, uint8_t behIdx)
{
    particleLab.clear();
    particleLab.behavior = (ParticleBehavior)(behIdx % 11);

    float cx = manualControlActive ? cursorX : 400.0f;
    float cy = manualControlActive ? cursorY : 250.0f;
    bool isRising = (particleLab.behavior == ParticleBehavior::REALISTIC_FIRE ||
                     particleLab.behavior == ParticleBehavior::SLIGHTLY_CHAOTIC ||
                     particleLab.behavior == ParticleBehavior::GRAVITY_BOUNCE);

    switch (shapeIdx % 6) {
        case 0: {
            float lineY = isRising ? (manualControlActive ? cy : 415.0f) : (manualControlActive ? cy : 85.0f);
            float halfSpan = (particleLab.behavior == ParticleBehavior::REALISTIC_FIRE) ? 68.0f : 220.0f;
            particleLab.setLineEmitter(cx - halfSpan, lineY, cx + halfSpan, lineY);
            particleLab.regionMode = EmitRegionMode::EDGE_PERIMETER;
            break;
        }
        case 1:
            particleLab.setCircleEmitter(cx, isRising && !manualControlActive ? 360.0f : cy, 75.0f, EmitRegionMode::EDGE_PERIMETER);
            break;
        case 2:
            particleLab.setCircleEmitter(cx, isRising && !manualControlActive ? 360.0f : cy, 80.0f, EmitRegionMode::SURFACE_AREA);
            break;
        case 3:
            particleLab.setRectEmitter(cx - 110, (isRising && !manualControlActive ? 340.0f : cy) - 65, 220, 130, EmitRegionMode::EDGE_PERIMETER);
            break;
        case 4:
            particleLab.setTriangleEmitter(cx, cy - 95, cx - 115, cy + 85, cx + 115, cy + 85, EmitRegionMode::EDGE_PERIMETER);
            break;
        case 5:
            particleLab.setRingEmitter(cx, cy, 45.0f, 95.0f, EmitRegionMode::SURFACE_AREA);
            break;
    }

    // Default size & lifecycle
    particleLab.startSize = 4.0f;
    particleLab.endSize   = 1.0f;
    particleLab.minLife   = 0.9f;
    particleLab.maxLife   = 1.75f;

    switch (particleLab.behavior) {
        case ParticleBehavior::REALISTIC_FIRE:
            particleLab.baseAngleDeg = -90.0f;
            particleLab.baseSpeed    = 145.0f;
            particleLab.startSize    = 22.0f;
            particleLab.endSize      = 4.0f;
            particleLab.minLife      = 0.85f;
            particleLab.maxLife      = 1.55f;
            particleLab.windX        = 35.0f * sinf(millis() * 0.001f);
            particleLab.renderStyle  = ParticleRenderStyle::FIRE_PLUME;
            break;
        case ParticleBehavior::PARALLEL:
            particleLab.baseAngleDeg = 90.0f;
            particleLab.baseSpeed = 340.0f;
            particleLab.renderStyle = ParticleRenderStyle::VELOCITY_STREAK;
            particleLab.startColor = Colors565::NeonCyan;
            particleLab.endColor = Colors565::RoyalBlue;
            break;
        case ParticleBehavior::PARALLEL_WIND:
            particleLab.baseAngleDeg = 90.0f;
            particleLab.baseSpeed = 310.0f;
            particleLab.windX = -240.0f * globalIntensity;
            particleLab.renderStyle = ParticleRenderStyle::VELOCITY_STREAK;
            particleLab.startColor = Colors565::SkyBlue;
            particleLab.endColor = Colors565::White;
            break;
        case ParticleBehavior::SLIGHTLY_CHAOTIC:
            particleLab.baseAngleDeg = -90.0f;
            particleLab.baseSpeed = 165.0f;
            particleLab.chaosStrength = 240.0f * globalIntensity;
            particleLab.renderStyle = ParticleRenderStyle::FIRE_PLUME;
            particleLab.startSize = 14.0f;
            particleLab.startColor = Colors565::Gold;
            particleLab.endColor = Colors565::Crimson;
            break;
        case ParticleBehavior::TOTALLY_CHAOTIC:
            particleLab.baseSpeed = 240.0f * globalIntensity;
            particleLab.chaosStrength = 420.0f * globalIntensity;
            particleLab.renderStyle = ParticleRenderStyle::VELOCITY_STREAK;
            particleLab.startColor = Colors565::ElectricYellow;
            particleLab.endColor = Colors565::NeonPink;
            break;
        case ParticleBehavior::OUTWARD_NORMAL:
            particleLab.baseSpeed = 200.0f * globalIntensity;
            particleLab.renderStyle = ParticleRenderStyle::ADDITIVE_GLOW;
            particleLab.startColor = Colors565::NeonGreen;
            particleLab.endColor = Colors565::Teal;
            break;
        case ParticleBehavior::INWARD_IMPLODE:
            particleLab.baseSpeed = 190.0f * globalIntensity;
            particleLab.renderStyle = ParticleRenderStyle::VELOCITY_STREAK;
            particleLab.startColor = Colors565::NeonPurple;
            particleLab.endColor = Colors565::White;
            break;
        case ParticleBehavior::VORTEX_SWIRL:
            particleLab.baseSpeed = 220.0f * globalIntensity;
            particleLab.renderStyle = ParticleRenderStyle::ADDITIVE_GLOW;
            particleLab.startColor = Colors565::NeonCyan;
            particleLab.endColor = Colors565::Amethyst;
            break;
        case ParticleBehavior::HARMONIC_WAVE:
            particleLab.baseAngleDeg = 0.0f;
            particleLab.baseSpeed = 210.0f * globalIntensity;
            particleLab.renderStyle = ParticleRenderStyle::FILLED_CIRCLE;
            particleLab.startColor = Colors565::Gold;
            particleLab.endColor = Colors565::NeonPink;
            break;
        case ParticleBehavior::GRAVITY_BOUNCE:
            particleLab.baseAngleDeg = -80.0f;
            particleLab.baseSpeed = 340.0f * globalIntensity;
            particleLab.renderStyle = ParticleRenderStyle::FILLED_CIRCLE;
            particleLab.startColor = Colors565::Orange;
            particleLab.endColor = Colors565::Crimson;
            break;
        case ParticleBehavior::HOMING_ATTRACTOR:
            particleLab.baseSpeed = 240.0f * globalIntensity;
            particleLab.renderStyle = ParticleRenderStyle::VELOCITY_STREAK;
            particleLab.startColor = Colors565::Emerald;
            particleLab.endColor = Colors565::White;
            break;
    }
}

void resetSceneObjects()
{
    cushionLab.init(185, 290, 430, 145, Colors565::Crimson, Colors565::Gold);
    cushionBallX  = 400.0f;
    cushionBallY  = 110.0f;
    cushionBallVx = 85.0f;
    cushionBallVy = 0.0f;

    const uint16_t silkColors[4] = { Colors565::Crimson, Colors565::Emerald, Colors565::RoyalBlue, Colors565::NeonPurple };
    clothLab.init(245, 72, 12.2f, silkColors[clothColorIdx % 4], Colors565::Gold);
    scarfLab.init(165, 225, 11.0f, Colors565::Crimson);
    waterLab.init(40, 355, 720, 105);
    weatherLab.init(WeatherType::SAKURA_PETALS);
    springLab.init(655, 82, 705, 245, 105.0f, 165.0f, 0.25f);
    ballA.init(180, 120, 26, 2.0f, 0.88f, Colors565::NeonOrange);
    ballA.vel = Vec2(240, -60);
    ballB.init(420, 100, 32, 3.0f, 0.88f, Colors565::ElectricBlue);
    ballB.vel = Vec2(-190, 60);
    shatterLab.clear();
    configureParticlePreset(particleShapeIdx, particleBehaviorIdx);
}

void drawTopBanner(const char *title, const char *subtitle)
{
    engine.fillRect(0, 0, 800, 44, Colors565::Obsidian);
    engine.drawFastHLine(0, 44, 800, Colors565::ElectricBlue);
    engine.drawTextShadow(12, 7, title, Colors565::Gold, Colors565::Black, 2, 2);
    engine.drawText(12, 28, subtitle, Colors565::LightSkyBlue, 1);

    char rightBuf[48];
    snprintf(rightBuf, sizeof(rightBuf), "S%d/9 Int:%.1fx [%s]", currentScene, globalIntensity, autoCycleScenes ? "AUTO" : "CTRL");
    engine.drawText(545, 14, rightBuf, Colors565::NeonGreen, 2);
}

// ============================================================================
// SCENE 1: ALL SHAPES, SPLINES, STARS, RINGS & TRUE CUBIC 3D WIREFRAMES
// ============================================================================
void renderScene1_Shapes(float t)
{
    engine.clear(Colors565::Charcoal);
    drawTopBanner("1. SHAPES & 3D GEOMETRY", "WASD: Rotate 3D Cube & Move Bezier | +/-: Scale & Thickness Intensity");

    int16_t thick = max(1, (int)(3.0f * globalIntensity));

    // Rounded Rect & Thick Rect
    engine.fillRoundRect(30, 65, 130, 80, 18, Colors565::RoyalBlue);
    engine.drawRoundRect(30, 65, 130, 80, 18, Colors565::White);
    engine.drawThickRect(185, 65, 120, 80, thick + 2, Colors565::NeonOrange);

    // Ring & ArcRing Gauge (Angle controlled by Intensity & Time)
    engine.fillRing(380, 105, 22, 42, Colors565::Emerald);
    float sweep = fmodf(t * 90.0f * globalIntensity, 350.0f) + 10.0f;
    engine.fillArcRing(500, 105, 24, 44, 0.0f, sweep, Colors565::NeonPink);

    // Rotating Star (Inner radius controlled by Intensity) & Hexagon
    int16_t innerR = constrain((int)(18.0f * globalIntensity), 8, 40);
    engine.fillStar(630, 105, 5, 45, innerR, t * 45.0f, Colors565::Gold);
    engine.drawStar(630, 105, 5, 45, innerR, t * 45.0f, Colors565::White);
    engine.fillRegularPolygon(735, 105, 6, 40, -t * 35.0f, Colors565::Amethyst);

    // Capsule, Diamond & Concave Polygon
    engine.fillCapsule(50, 210, 160, 235, 20, Colors565::Turquoise);
    engine.fillDiamond(260, 220, 45, 35, Colors565::Crimson);
    Point concave[6] = { {340,185}, {430,185}, {400,220}, {430,255}, {340,255}, {365,220} };
    engine.fillPolygon(concave, 6, Colors565::SpringGreen);

    // Interactive Cubic Bezier (Control point follows cursorY)
    int16_t waveY = manualControlActive ? (int16_t)(cursorY - 240) : (int16_t)(sinf(t * 3.0f) * 45.0f * globalIntensity);
    engine.drawCubicBezier(40, 340, 140, 260 + waveY, 240, 420 - waveY, 340, 340, Colors565::ElectricYellow, thick);
    engine.drawArrow(40, 420, 320, 420, 16, Colors565::NeonCyan, thick);
    engine.drawAntiAliasedLine(40, 445, 320, 465, Colors565::White);

    // True Cubic Telephoto 3D Cube, Pyramid & Sphere (Rotated via WASD or time)
    float rx = manualControlActive ? (cursorY * 0.015f) : (t * 0.9f);
    float ry = manualControlActive ? (cursorX * 0.015f) : (t * 1.3f);
    float cubeScale = 75.0f * constrain(globalIntensity, 0.5f, 1.4f);

    engine.drawCube3D(495, 365, cubeScale, rx, ry, 0.0f, Colors565::NeonCyan);
    engine.drawPyramid3D(635, 365, cubeScale, 0.35f, -ry, 0.0f, Colors565::Gold);
    engine.drawSphereGrid3D(745, 365, cubeScale * 0.55f, rx, ry, Colors565::NeonPink);
}

// ============================================================================
// SCENE 2: COLOR LIBRARY, HSV SPECTRUM, WAVELENGTH & KELVIN HEAT PHYSICS
// ============================================================================
void renderScene2_ColorPhysics(float t)
{
    engine.clear(Colors565::Obsidian);
    drawTopBanner("2. COLOUR LIBRARY & LIGHT PHYSICS", "WASD: Scrub Spectrum Cursor | +/-: Brightness/Saturation Intensity");

    uint8_t val = (uint8_t)constrain((int)(globalIntensity * 180.0f), 40, 255);

    // 1. HSV Hue Spectrum Bar
    engine.drawText(30, 58, "HSV Hue Wheel (0-360 deg):", Colors565::White, 2);
    for (int x = 0; x < 740; x += 2) {
        uint16_t hue = (uint16_t)(((x * 360) / 740 + (int)(t * 60.0f)) % 360);
        uint16_t col = Color::hsv565(hue, 255, val);
        engine.drawFastVLine(30 + x, 80, 46, col);
        engine.drawFastVLine(31 + x, 80, 46, col);
    }

    // 2. Visible Light Wavelength Physics (380nm Violet to 750nm Red)
    engine.drawText(30, 140, "Physical Light Wavelength Spectrum (380nm - 750nm):", Colors565::White, 2);
    for (int x = 0; x < 740; x += 2) {
        float nm = 380.0f + (370.0f * x) / 740.0f;
        uint16_t col = Color::scaleBrightness565(Color::wavelength565(nm), val);
        engine.drawFastVLine(30 + x, 162, 46, col);
        engine.drawFastVLine(31 + x, 162, 46, col);
    }

    // 3. Blackbody Radiation Kelvin Temperature (1000K Ember -> 12000K Blue Star)
    engine.drawText(30, 224, "Blackbody Thermal Radiation (1000K Ember -> 5500K Sun -> 12000K Star):", Colors565::White, 2);
    for (int x = 0; x < 740; x += 2) {
        uint16_t kelvin = 1000 + (uint16_t)((11000L * x) / 740);
        uint16_t col = Color::scaleBrightness565(Color::kelvin565(kelvin), val);
        engine.drawFastVLine(30 + x, 246, 46, col);
        engine.drawFastVLine(31 + x, 246, 46, col);
    }

    // Interactive Cursor Readout on the Spectrums
    int16_t cx = constrain((int16_t)cursorX, (int16_t)30, (int16_t)769);
    engine.drawFastVLine(cx, 75, 222, Colors565::White);
    float curNm = 380.0f + (370.0f * (cx - 30)) / 740.0f;
    uint16_t curK = 1000 + (uint16_t)((11000L * (cx - 30)) / 740);
    char specInfo[64];
    snprintf(specInfo, sizeof(specInfo), "Cursor: %.0fnm | %uK Blackbody", curNm, curK);
    engine.drawTextShadow(30, 305, specInfo, Colors565::Gold, Colors565::Black, 2);

    // 4. Swatch Grid of Named Colors
    const uint16_t swatches[16] = {
        Colors565::Crimson, Colors565::Scarlet, Colors565::Tangerine, Colors565::Gold,
        Colors565::NeonGreen, Colors565::Emerald, Colors565::Jade, Colors565::Turquoise,
        Colors565::ElectricBlue, Colors565::RoyalBlue, Colors565::Cobalt, Colors565::Indigo,
        Colors565::Amethyst, Colors565::NeonPurple, Colors565::DeepPink, Colors565::SakuraPink
    };
    for (int i = 0; i < 16; i++) {
        int16_t sx = 30 + i * 46;
        engine.fillRoundRect(sx, 338, 42, 105, 6, Color::scaleBrightness565(swatches[i], val));
        engine.drawRoundRect(sx, 338, 42, 105, 6, Colors565::White);
    }
}

// ============================================================================
// SCENE 3: TRUE ALPHA TRANSPARENCY (OVER CHECKERBOARD) & 17 BLEND MODES
// ============================================================================
void renderScene3_Transparency(float t)
{
    engine.clear(Colors565::Obsidian);

    // 1. Draw a High-Contrast Checkerboard + Background Text so Alpha Transparency is 100% unmistakable!
    for (int y = 52; y < 325; y += 26) {
        for (int x = 20; x < 780; x += 26) {
            bool lightSq = (((x / 26) + (y / 26)) & 1) == 0;
            engine.fillRect(x, y, 26, 26, lightSq ? Colors565::WhiteSmoke : Colors565::Gunmetal);
        }
    }
    // Bold opaque reference stripes & text painted directly onto the background Layer 0
    engine.fillRect(20, 145, 760, 26, Colors565::Gold);
    engine.drawTextShadow(45, 150, "BACKGROUND LAYER VISIBLE THROUGH ALPHA!", Colors565::Black, Colors565::White, 2, 1);

    const char *lensModeNames[10] = {
        "GLASS LENS", "ALPHA BLEND", "ADDITIVE GLOW", "MULTIPLY FILTER", "SCREEN BLOOM",
        "OVERLAY CONTRAST", "SOFT LIGHT", "DIFFERENCE XRAY", "SUBTRACTIVE VOID", "BAYER STIPPLE"
    };
    const BlendMode lensModes[10] = {
        BlendMode::ALPHA, BlendMode::ALPHA, BlendMode::ADDITIVE, BlendMode::MULTIPLY, BlendMode::SCREEN,
        BlendMode::OVERLAY, BlendMode::SOFT_LIGHT, BlendMode::DIFFERENCE, BlendMode::SUBTRACTIVE, BlendMode::CHECKER_STIPPLE
    };

    // Keep alpha in the sweet transparent glass range so it never turns 100% solid opaque!
    uint8_t alphaAmt = (uint8_t)constrain((int)(globalIntensity * 125.0f), 40, 215);
    uint8_t glassFilterAmt = (uint8_t)constrain((int)(globalIntensity * 175.0f), 70, 235);
    uint8_t glassTintAmt   = (uint8_t)constrain((int)(globalIntensity * 55.0f),  20, 95);
    int opacityPct = (alphaAmt * 100) / 255;

    char sub[115];
    snprintf(sub, sizeof(sub), "Lens:[%s] | Opacity:%d%% | WASD:Move Lens | M/SPACE:Cycle 10 Modes | +/-:Alpha",
             lensModeNames[blendLensModeIdx % 10], opacityPct);
    drawTopBanner("3. TRUE ALPHA TRANSPARENCY & BLEND MODES", sub);

    int16_t shift = (int16_t)(sinf(t * 3.0f) * 32.0f);

    // 2. Left: Optical Stained-Glass Alpha Circles over the Grid (Cyan + Yellow -> Green, Yellow + Magenta -> Orange-Red, Cyan + Magenta -> Violet!)
    int16_t c1x = 135 - shift, c1y = 162;
    int16_t c2x = 195 + shift, c2y = 162;
    int16_t c3x = 165,         c3y = 214 - shift / 2;
    uint16_t colCyan    = RGB565_CONST(0, 245, 255);
    uint16_t colYellow  = RGB565_CONST(255, 245, 0);
    uint16_t colMagenta = RGB565_CONST(255, 25, 195);

    // Optical stained-glass filter + translucent alpha sheen so both the grid AND the overlapping secondary colors blend vividly!
    engine.fillCircleBlend(c1x, c1y, 66, colCyan,    glassFilterAmt, BlendMode::MULTIPLY);
    engine.fillCircleAlpha(c1x, c1y, 66, colCyan,    glassTintAmt);

    engine.fillCircleBlend(c2x, c2y, 66, colYellow,  glassFilterAmt, BlendMode::MULTIPLY);
    engine.fillCircleAlpha(c2x, c2y, 66, colYellow,  glassTintAmt);

    engine.fillCircleBlend(c3x, c3y, 66, colMagenta, glassFilterAmt, BlendMode::MULTIPLY);
    engine.fillCircleAlpha(c3x, c3y, 66, colMagenta, glassTintAmt);

    engine.drawCircle(c1x, c1y, 66, Colors565::White);
    engine.drawCircle(c2x, c2y, 66, Colors565::White);
    engine.drawCircle(c3x, c3y, 66, Colors565::White);
    engine.drawTextShadow(85, 298, "ALPHA OVER GRID", Colors565::White, Colors565::Black, 2);

    // 3. Center: Optical Additive Color Mixing over dark circular stage (R + G + B -> Yellow, Cyan, Magenta, White!)
    engine.fillCircle(422, 188, 108, Colors565::Obsidian);
    engine.drawCircle(422, 188, 108, Colors565::Silver);
    uint8_t addAmt = (uint8_t)constrain((int)(globalIntensity * 145.0f), 45, 255);
    engine.fillCircleBlend(395 - shift, 168, 62, Colors565::Red,       addAmt, BlendMode::ADDITIVE);
    engine.fillCircleBlend(450 + shift, 168, 62, Colors565::PureGreen, addAmt, BlendMode::ADDITIVE);
    engine.fillCircleBlend(422, 218 - shift / 2, 62, Colors565::Blue,  addAmt, BlendMode::ADDITIVE);
    engine.drawTextShadow(345, 298, "RGB LIGHT BLEND", Colors565::Gold, Colors565::Black, 2);

    // 4. Bottom Live Showcase Strip of 6 Studio Blend Modes!
    const BlendMode stripModes[6] = {
        BlendMode::ALPHA, BlendMode::MULTIPLY, BlendMode::SCREEN,
        BlendMode::OVERLAY, BlendMode::DIFFERENCE, BlendMode::CHECKER_STIPPLE
    };
    const uint16_t stripCols[6] = {
        Colors565::ElectricBlue, Colors565::Gold, Colors565::NeonPink,
        Colors565::NeonCyan, Colors565::ElectricYellow, Colors565::NeonGreen
    };
    const char *stripLabels[6] = { "ALPHA", "MULT", "SCRN", "OVRLY", "DIFF", "STIP" };

    for (int i = 0; i < 6; i++) {
        int16_t bx = 25 + i * 126;
        int16_t by = 345;
        engine.fillLinearGradientRect(bx, by, 114, 115, Color(230, 65, 95), Color(35, 195, 255), false, false);
        engine.fillRect(bx + 4, by + 36, 106, 16, Colors565::White);
        engine.fillRoundRectBlend(bx + 12, by + 10, 90, 72, 12, stripCols[i], alphaAmt, stripModes[i]);
        engine.drawRoundRect(bx + 12, by + 10, 90, 72, 12, Colors565::White);
        engine.drawTextShadow(bx + 22, by + 90, stripLabels[i], Colors565::White, Colors565::Black, 2);
    }

    // 5. Interactive Movable Multi-Mode Glass Window / Lens (Move anywhere over the grid with WASD!)
    int16_t lx = manualControlActive ? (int16_t)cursorX : (int16_t)(655 + cosf(t * 2.0f) * 45.0f);
    int16_t ly = manualControlActive ? (int16_t)cursorY : (int16_t)(185 + sinf(t * 2.0f) * 35.0f);

    if ((blendLensModeIdx % 10) == 0) {
        engine.drawGlassLensCircle(lx, ly, 74, Colors565::ElectricBlue, alphaAmt, 0.35f);
    } else {
        BlendMode activeMode = lensModes[blendLensModeIdx % 10];
        engine.drawDropShadowRect(lx - 82, ly - 82, 164, 164, 18, 8, 10, 2, 120);
        engine.fillRoundRectBlend(lx - 82, ly - 82, 164, 164, 18, Colors565::NeonCyan, alphaAmt, activeMode);
        engine.fillVerticalAlphaGradientRect(lx - 76, ly - 76, 152, 42, Colors565::White, 115, 5);
        engine.drawRoundRect(lx - 82, ly - 82, 164, 164, 18, Colors565::White);
    }
    engine.drawTextShadow(lx - 58, ly - 6, lensModeNames[blendLensModeIdx % 10], Colors565::White, Colors565::Black, 2);
}

// ============================================================================
// SCENE 4: DYNAMIC LIGHTS, PHOTOREALISTIC GLOWING STICK & INTENSITY SHADOWS
// ============================================================================
void renderScene4_LightingAndShadows(float t)
{
    engine.fillLinearGradientRect(0, 45, 800, 435, Color(24, 30, 54), Color(48, 36, 68), true, false);
    drawTopBanner("4. LIGHTS, NEON STICK & RAY SHADOWS", "WASD: Move Light/Stick | M: Toggle Light/Stick | +/-: Light & Shadow Intensity");

    // Orbiting or WASD-controlled Point Light source
    float lx = (!manualControlActive || moveStickInScene4) ? (250.0f + cosf(t * 2.2f) * 145.0f) : cursorX;
    float ly = (!manualControlActive || moveStickInScene4) ? (255.0f + sinf(t * 2.2f) * 95.0f)  : cursorY;

    float c1x = 170, c1y = 200, c1r = 32;
    float c2x = 340, c2y = 310, c2r = 38;
    float bx = 220, by = 120, bw = 65, bh = 45;

    // 1. Render Single-Pass Point Light first so shadows cast over the ambient + lit floor
    LightSystem2D::drawPointLight(engine, lx, ly, 175.0f, Colors565::Amber, globalIntensity);

    // 2. Cast Soft Umbra + Penumbra Shadows whose darkness is dynamically controlled by globalIntensity & distance!
    LightSystem2D::castCircleShadow(engine, lx, ly, c1x, c1y, c1r, 275.0f, 155, globalIntensity);
    LightSystem2D::castCircleShadow(engine, lx, ly, c2x, c2y, c2r, 275.0f, 155, globalIntensity);
    LightSystem2D::castBoxShadow(engine, lx, ly, bx, by, bw, bh, 275.0f, 155, globalIntensity);

    engine.fillCircle((int16_t)lx, (int16_t)ly, 6, Colors565::White);

    // 3. Draw physical obstacles
    engine.fillRadialGradientCircle((int16_t)c1x, (int16_t)c1y, (int16_t)c1r, Color(80, 220, 255), Color(10, 40, 90), false);
    engine.fillRadialGradientCircle((int16_t)c2x, (int16_t)c2y, (int16_t)c2r, Color(255, 110, 150), Color(80, 10, 30), false);
    engine.fillRect((int)bx, (int)by, (int)bw, (int)bh, Colors565::SlateGray);
    engine.drawRect((int)bx, (int)by, (int)bw, (int)bh, Colors565::White);

    // 4. Photorealistic SDF Glowing Neon Stick / Lightsaber + Area Light + Gouraud Triangle
    float stickCx = (manualControlActive && moveStickInScene4) ? cursorX : 635.0f;
    float stickCy = (manualControlActive && moveStickInScene4) ? cursorY : 135.0f;
    float stickAngle = t * 1.2f;
    float sx0 = stickCx - cosf(stickAngle) * 95.0f;
    float sy0 = stickCy - sinf(stickAngle) * 35.0f;
    float sx1 = stickCx + cosf(stickAngle) * 95.0f;
    float sy1 = stickCy + sinf(stickAngle) * 35.0f;

    LightSystem2D::drawLineLight(engine, sx0, sy0, sx1, sy1, 24.0f, Colors565::NeonPink, globalIntensity);
    LightSystem2D::drawRectAreaLight(engine, 565, 220, 140, 42, 22, Colors565::NeonCyan, globalIntensity);
    engine.fillTriangleGouraud(540, 435, Colors565::Red, 640, 305, Colors565::PureGreen, 745, 435, Colors565::Blue);
}

// ============================================================================
// SCENE 5: REALISTIC CONTIGUOUS FRACTURE & SHATTER LAB (ZERO RING ARTIFACTS)
// ============================================================================
static uint32_t shatterTriggerMs = 0;

void triggerCurrentShatter(float hitX, float hitY)
{
    uint16_t matColors[5] = {
        Colors565::ElectricBlue, Colors565::Silver, Colors565::Peru, Colors565::Gold, Colors565::Turquoise
    };
    shatterLab.triggerShatter(
        400.0f, 215.0f, 92.0f,
        hitX, hitY,
        (ShatterMaterial)shatterMatIdx,
        globalIntensity,
        matColors[shatterMatIdx]
    );
    shatterTriggerMs = millis();
}

void renderScene5_ShatterLab(float t, float dt)
{
    (void)t;
    engine.clear(Colors565::Obsidian);

    const char *matNames[5] = { "GLASS PANE", "STONE SLAB", "WOOD CRATE", "HEATED STEEL", "CERAMIC TILE" };
    uint16_t matColors[5] = {
        Colors565::ElectricBlue, Colors565::Silver, Colors565::Peru, Colors565::Gold, Colors565::Turquoise
    };

    char sub[105];
    snprintf(sub, sizeof(sub), "Mat:%s | WASD:Aim Impact | SPACE:Break | R:Reform | M:Mat | +/-:Force", matNames[shatterMatIdx]);
    drawTopBanner("5. REALISTIC CONTIGUOUS FRACTURE LAB", sub);

    // Draw solid stone floor where broken shards clatter and pile up
    engine.fillRect(0, 438, 800, 42, Colors565::DarkSlateGray);
    engine.drawFastHLine(0, 438, 800, Colors565::Silver);

    // In auto demo mode: show intact pane for 1.0s, then fracture at a random impact point, let shards pile on floor for 2.2s
    uint32_t elapsed = millis() - shatterTriggerMs;
    if (!manualControlActive) {
        if (!shatterLab.active && elapsed > 1000) {
            triggerCurrentShatter(400.0f + random(-55, 56), 215.0f + random(-45, 46));
        } else if (shatterLab.active && elapsed > 3200) {
            shatterLab.clear();
            shatterMatIdx = (shatterMatIdx + 1) % 5;
            shatterTriggerMs = millis();
        }
    }

    if (!shatterLab.active) {
        // Render the intact window pane / stone slab / wooden crate waiting to be broken
        ShatterSystem2D::drawIntactObject(engine, 400.0f, 215.0f, 92.0f, (ShatterMaterial)shatterMatIdx, matColors[shatterMatIdx]);
    } else {
        shatterLab.update(dt);
        shatterLab.draw(engine);
    }

    // Small impact aim reticle (no expanding ring animations!)
    if (manualControlActive) {
        engine.drawCrosshair((int16_t)cursorX, (int16_t)cursorY, 10, Colors565::NeonRed);
    }
}

// ============================================================================
// SCENE 6: REALISTIC BOUNCY PLUSH CUSHION & TWO-WAY ELASTIC BALL
// ============================================================================
void renderScene6_CushionCompression(float t, float dt)
{
    (void)t;
    engine.clear(Colors565::Charcoal);
    drawTopBanner("6. BOUNCY PLUSH CUSHION & COMPRESSION", "Auto-Bounce or WASD:Push Ball Deep | SPACE:Drop/Bounce | +/-:Elasticity");

    const float ballRadius = 40.0f;

    if (manualControlActive) {
        // Manual mode: WASD directly controls the ball position to press & roll across the plush pillow!
        cushionBallX  = constrain(cursorX, 200.0f, 600.0f);
        cushionBallY  = constrain(cursorY, 85.0f, 385.0f);
        cushionBallVy = velY;
        cushionLab.interactBall(cushionBallX, cushionBallY, cushionBallVy, ballRadius, dt, true, globalIntensity);
    } else {
        // Realistic physical free-fall & elastic cushion rebound!
        float gravity = 980.0f * constrain(globalIntensity, 0.5f, 2.0f);
        cushionBallVy += gravity * dt;
        cushionBallX  += cushionBallVx * dt;
        cushionBallY  += cushionBallVy * dt;

        // Bounce gently between left & right edges of the cushion
        if (cushionBallX < 250.0f) { cushionBallX = 250.0f; cushionBallVx = fabsf(cushionBallVx); }
        if (cushionBallX > 550.0f) { cushionBallX = 550.0f; cushionBallVx = -fabsf(cushionBallVx); }

        // Two-way elastic interaction: ball indents cushion, and compressed cushion springs launch ball back up!
        cushionLab.interactBall(cushionBallX, cushionBallY, cushionBallVy, ballRadius, dt, false, globalIntensity);

        // Keep ball looping high if bounce decays after many bounces
        if (cushionBallY > 242.0f && fabsf(cushionBallVy) < 45.0f) {
            cushionBallVy = -460.0f * constrain(globalIntensity, 0.6f, 1.8f);
        }
    }

    cushionLab.update(dt);
    cushionLab.draw(engine);

    // Draw the heavy polished sphere pressing/bouncing on the cushion
    engine.fillRadialGradientCircle((int16_t)cushionBallX, (int16_t)cushionBallY, (int16_t)ballRadius, Color(255, 255, 255), Color(25, 135, 255), false);
    engine.drawCircle((int16_t)cushionBallX, (int16_t)cushionBallY, (int16_t)ballRadius, Colors565::White);
}

// ============================================================================
// SCENE 7: SILKY 3D-SHADED VELVET CURTAIN & FLOWING NINJA SCARF
// ============================================================================
void renderScene7_ClothAndScarf(float t, float dt)
{
    engine.clear(Colors565::Obsidian);
    char sub[105];
    snprintf(sub, sizeof(sub), "WASD:Move Sphere | SPACE:Mode[%s] | M:Silk Color | R:Repair | +/-:Wind", clothTearMode ? "BLADE TEAR" : "SMOOTH PUSH");
    drawTopBanner("7. SILK DRAPE & NINJA SCARF PHYSICS", sub);

    float cx = manualControlActive ? cursorX : (415.0f + sinf(t * 1.8f) * 145.0f);
    float cy = manualControlActive ? cursorY : (195.0f + cosf(t * 1.3f) * 44.0f);

    // Draw 3D sphere behind the curtain so the high-res silk fabric visibly drapes & wraps over it!
    uint16_t ballCol = clothTearMode ? Colors565::NeonRed : Colors565::ElectricBlue;
    engine.fillRadialGradientCircle((int16_t)cx, (int16_t)cy, 38, Color(255, 255, 255), Color::fromRGB565(ballCol), false);

    clothLab.interactCircle(cx, cy, 42.0f, clothTearMode);
    clothLab.update(dt, 195.0f * globalIntensity * sinf(t * 1.3f), 680.0f);
    clothLab.draw(engine, true);

    // Subtle translucent glass indicator ring showing the sphere / blade position through the silk
    engine.fillCircleAlpha((int16_t)cx, (int16_t)cy, 38, ballCol, 65);
    engine.drawCircle((int16_t)cx, (int16_t)cy, 38, Colors565::White);

    float ninjaX = 135.0f;
    float ninjaY = 245.0f + sinf(t * 4.5f) * 34.0f;
    scarfLab.update(ninjaX, ninjaY, dt, -360.0f * globalIntensity, 180.0f);
    scarfLab.drawScarf(engine, 15.0f);
    engine.fillCircle((int16_t)ninjaX, (int16_t)ninjaY, 22, Colors565::White);
    engine.fillRect((int16_t)ninjaX - 22, (int16_t)ninjaY - 6, 44, 8, Colors565::Crimson);
}

// ============================================================================
// SCENE 8: RIGID-BODY BOUNCE, LIVELY SPRING, 2-BONE IK, WATER & LIGHTNING
// ============================================================================
void renderScene8_FullGamePhysics(float t, float dt)
{
    engine.clear(Colors565::Obsidian);
    drawTopBanner("8. GAME PHYSICS, SPRING, IK & WATER", "WASD:Move IK Hand & Hit Spring | SPACE:Yank Spring & Splash | +/-:Intensity");

    weatherLab.updateAndDraw(engine, dt);

    float prevAY = ballA.pos.y;
    ballA.integrate(dt, 900.0f * globalIntensity, 430.0f, 40.0f, 760.0f);
    ballB.integrate(dt, 900.0f * globalIntensity, 430.0f, 40.0f, 760.0f);
    RigidBody2D::resolveCircleCollision(ballA, ballB);

    // Keep balls lively if they ever slow down
    if (ballA.vel.lengthSq() < 12000.0f) ballA.vel = Vec2(230.0f, -460.0f * globalIntensity);

    if (prevAY < 355.0f && ballA.pos.y >= 355.0f) {
        waterLab.splash(ballA.pos.x, 190.0f * globalIntensity);
    }

    // Two-way physical collisions between bouncing balls and the Spring Bob!
    springLab.collideWithCircle(ballA, 22.0f);
    springLab.collideWithCircle(ballB, 22.0f);

    // Also allow the IK hand / WASD cursor to swat and bounce the spring bob!
    Vec2 handTarget = manualControlActive ? Vec2(cursorX, cursorY) : ballA.pos;
    Vec2 handToBob = springLab.bob - handTarget;
    if (handToBob.lengthSq() < 34.0f * 34.0f) {
        springLab.applyImpulse(handToBob.normalized() * (320.0f * globalIntensity));
    }

    // Ensure the spring bob is ALWAYS actively bouncing with harmonic energy!
    if (springLab.vel.lengthSq() < 2500.0f) {
        springLab.applyImpulse(Vec2(sinf(t * 3.0f) * 140.0f, -290.0f * globalIntensity));
    }
    springLab.update(dt, 680.0f);

    swordTrail.addPoint(handTarget.x, handTarget.y);
    swordTrail.draw(engine, 16.0f * globalIntensity, Colors565::Gold, Colors565::Crimson);

    engine.fillCircle((int16_t)ballA.pos.x, (int16_t)ballA.pos.y, (int16_t)ballA.radius, ballA.color);
    engine.drawCircle((int16_t)ballA.pos.x, (int16_t)ballA.pos.y, (int16_t)ballA.radius, Colors565::White);
    engine.fillCircle((int16_t)ballB.pos.x, (int16_t)ballB.pos.y, (int16_t)ballB.radius, ballB.color);
    engine.drawCircle((int16_t)ballB.pos.x, (int16_t)ballB.pos.y, (int16_t)ballB.radius, Colors565::White);

    // Draw lively bouncing spring & bob
    springLab.draw(engine, 11, 14.0f, Colors565::Silver);
    engine.fillRadialGradientCircle((int16_t)springLab.bob.x, (int16_t)springLab.bob.y, 22, Color(100, 255, 160), Color(10, 110, 50), false);
    engine.drawCircle((int16_t)springLab.bob.x, (int16_t)springLab.bob.y, 22, Colors565::White);

    Vec2 shoulder(120, 180);
    Vec2 elbow = IK2D::solveTwoBoneIK(shoulder, handTarget, 95.0f, 95.0f, true);
    engine.drawThickLine((int)shoulder.x, (int)shoulder.y, (int)elbow.x, (int)elbow.y, 7, Colors565::Gold, true);
    engine.drawThickLine((int)elbow.x, (int)elbow.y, (int)handTarget.x, (int)handTarget.y, 5, Colors565::NeonCyan, true);
    engine.fillCircle((int16_t)shoulder.x, (int16_t)shoulder.y, 8, Colors565::White);
    engine.fillCircle((int16_t)elbow.x, (int16_t)elbow.y, 6, Colors565::Crimson);

    if (((int)(t * 10.0f)) % 3 == 0) {
        LightSystem2D::drawLightningBolt(engine, 300, 75, 540, 95, 28.0f * globalIntensity, Colors565::ElectricBlue, 4);
    }

    waterLab.update(dt);
    waterLab.draw(engine, Colors565::DodgerBlue, 145);

    float hp = 65.0f + 30.0f * sinf(t * 1.5f);
    UIWidgets::drawHealthBar(engine, 30, 58, 220, 24, hp, hp + 15.0f, 100.0f, "HP");
    UIWidgets::drawCircularGauge(engine, 735, 300, 38, constrain(globalIntensity / 2.5f, 0.0f, 1.0f), Colors565::NeonGreen, "INT");
}

// ============================================================================
// SCENE 9: REALISTIC VOLUMETRIC FIRE PLUME & UNIVERSAL SHAPE PARTICLE LAB
// ============================================================================
void renderScene9_ParticleLab(float t, float dt)
{
    static uint32_t lastSwitchMs = 0;
    engine.clear(Colors565::Obsidian);

    if (autoCycleScenes && !manualControlActive && (millis() - lastSwitchMs > 3800)) {
        lastSwitchMs = millis();
        particleBehaviorIdx = (particleBehaviorIdx + 1) % 11;
        configureParticlePreset(particleShapeIdx, particleBehaviorIdx);
    }

    const char *shapeNames[6] = {
        "LINE(Hearth)", "CIRCLE(Edge)", "CIRCLE(Surf)", "RECT(Edge)", "TRIANGLE(Edge)", "RING(Surf)"
    };
    const char *behNames[11] = {
        "REALISTIC FIRE", "PARALLEL RAIN", "WIND RAIN", "EMBER PLUME", "TOTAL CHAOS",
        "OUTWARD NORM", "INWARD IMPLODE", "VORTEX SWIRL", "HARMONIC WAVE", "GRAV FOUNTAIN", "HOMING TARGET"
    };

    char sub[115];
    snprintf(sub, sizeof(sub), "Mode:%s | Shape:%s | B/SPACE:Next Mode | M:Shape | WASD:Move | +/-:Int",
             behNames[particleBehaviorIdx % 11], shapeNames[particleShapeIdx % 6]);
    drawTopBanner("9. VOLUMETRIC FIRE & SHAPE PARTICLE LAB", sub);

    particleLab.spawnRate = ((particleLab.behavior == ParticleBehavior::REALISTIC_FIRE) ? 92.0f : 75.0f) * globalIntensity;
    particleLab.windX = (particleLab.behavior == ParticleBehavior::REALISTIC_FIRE)
                        ? (sinf(t * 1.8f) * 95.0f * globalIntensity)
                        : (-240.0f * globalIntensity);
    particleLab.chaosStrength = 280.0f * globalIntensity;

    if (manualControlActive) {
        if (particleLab.behavior == ParticleBehavior::HOMING_ATTRACTOR) {
            particleLab.attractorX = cursorX;
            particleLab.attractorY = cursorY;
        } else if (particleLab.shape == EmitterShape::LINE) {
            float halfSpan = (particleLab.behavior == ParticleBehavior::REALISTIC_FIRE) ? 68.0f : 220.0f;
            particleLab.x  = cursorX - halfSpan;
            particleLab.x2 = cursorX + halfSpan;
            particleLab.y  = cursorY;
            particleLab.y2 = cursorY;
        } else {
            particleLab.x = cursorX;
            particleLab.y = cursorY;
        }
    } else {
        particleLab.attractorX = 400.0f + cosf(t * 2.2f) * 240.0f;
        particleLab.attractorY = 260.0f + sinf(t * 4.4f) * 110.0f;
    }

    particleLab.update(dt);
    particleLab.draw(engine);
}

// ============================================================================
// DUAL SERIAL INPUT PARSER (Reads from BOTH USB-CDC 'Serial' & UART0 'Serial0')
// ============================================================================
void processCommandChar(char c)
{
    lastInputMs = millis();

    // 1..9 Scene Selection
    if (c >= '1' && c <= '9') {
        currentScene = c - '0';
        autoCycleScenes = false;
        Serial.printf("Switched to Scene %d\n", currentScene);
        Serial0.printf("Switched to Scene %d\n", currentScene);
    }
    else if (c == '0') {
        autoCycleScenes = !autoCycleScenes;
        sceneStartTime = millis();
        Serial.printf("Auto-Cycle: %s\n", autoCycleScenes ? "ON" : "OFF");
        Serial0.printf("Auto-Cycle: %s\n", autoCycleScenes ? "ON" : "OFF");
    }
    else if (c == 'n' || c == 'N') {
        currentScene = (currentScene % 9) + 1;
        sceneStartTime = millis();
    }
    // WASD Live Cursor / Object Movement (from control.py or Serial Monitor)
    else if (c == 'w' || c == 'W') { manualControlActive = true; velY = -340.0f; cursorY = max(55.0f,  cursorY - 18.0f); }
    else if (c == 's' || c == 'S') { manualControlActive = true; velY =  340.0f; cursorY = min(465.0f, cursorY + 18.0f); }
    else if (c == 'a' || c == 'A') { manualControlActive = true; velX = -340.0f; cursorX = max(20.0f,  cursorX - 18.0f); }
    else if (c == 'd' || c == 'D') { manualControlActive = true; velX =  340.0f; cursorX = min(780.0f, cursorX + 18.0f); }
    else if (c == 'x' || c == 'X') { velX = 0.0f; velY = 0.0f; } // Key release stop
    // Universal Intensity Control ('+' / '-' or 'i' / 'k')
    else if (c == '+' || c == '=' || c == 'i' || c == 'I') {
        globalIntensity = min(2.5f, globalIntensity + 0.15f);
        Serial.printf("Intensity: %.2f\n", globalIntensity);
        Serial0.printf("Intensity: %.2f\n", globalIntensity);
    }
    else if (c == '-' || c == '_' || c == 'k' || c == 'K') {
        globalIntensity = max(0.15f, globalIntensity - 0.15f);
        Serial.printf("Intensity: %.2f\n", globalIntensity);
        Serial0.printf("Intensity: %.2f\n", globalIntensity);
    }
    // 'M' / 'E': Cycle Sub-Mode / Blend Lens / Material / Silk Color / Emitter Shape
    else if (c == 'm' || c == 'M' || c == 'e' || c == 'E') {
        if (currentScene == 3) {
            blendLensModeIdx = (blendLensModeIdx + 1) % 10;
        }
        else if (currentScene == 4) {
            moveStickInScene4 = !moveStickInScene4;
        }
        else if (currentScene == 5) {
            shatterMatIdx = (shatterMatIdx + 1) % 5;
            shatterLab.clear(); // Show intact pane of new material first!
        }
        else if (currentScene == 7) {
            clothColorIdx = (clothColorIdx + 1) % 4;
            const uint16_t silkColors[4] = { Colors565::Crimson, Colors565::Emerald, Colors565::RoyalBlue, Colors565::NeonPurple };
            clothLab.clothColor1 = silkColors[clothColorIdx];
        }
        else if (currentScene == 9) {
            particleShapeIdx = (particleShapeIdx + 1) % 6;
            configureParticlePreset(particleShapeIdx, particleBehaviorIdx);
        }
    }
    // 'B': Cycle Particle Behavior Pattern (or Blend Mode in Scene 3)
    else if (c == 'b' || c == 'B') {
        if (currentScene == 3) {
            blendLensModeIdx = (blendLensModeIdx + 1) % 10;
        } else {
            particleBehaviorIdx = (particleBehaviorIdx + 1) % 11;
            configureParticlePreset(particleShapeIdx, particleBehaviorIdx);
        }
    }
    // 'J' or SPACE: Action Trigger (Shatter / Drop-Bounce Cushion / Tear Cloth / Yank Spring & Splash / Next Particle Behavior)
    else if (c == ' ' || c == 'j' || c == 'J') {
        if (currentScene == 3) {
            blendLensModeIdx = (blendLensModeIdx + 1) % 10;
        }
        else if (currentScene == 5) {
            if (shatterLab.active) shatterLab.clear();
            else triggerCurrentShatter(manualControlActive ? cursorX : 400.0f, manualControlActive ? cursorY : 215.0f);
        }
        else if (currentScene == 6) {
            // Release manual hold and slam/bounce the ball onto the cushion!
            manualControlActive = false;
            cushionBallY = 235.0f;
            cushionBallVy = 620.0f * globalIntensity;
            cushionLab.applyPress(cushionBallX, 78.0f, 85.0f * globalIntensity);
        }
        else if (currentScene == 7) {
            clothTearMode = !clothTearMode;
        }
        else if (currentScene == 8) {
            waterLab.splash(cursorX, 280.0f * globalIntensity);
            ballA.vel.y = -520.0f * globalIntensity;
            springLab.applyImpulse(Vec2(240.0f, -540.0f * globalIntensity));
        }
        else if (currentScene == 9) {
            particleBehaviorIdx = (particleBehaviorIdx + 1) % 11;
            configureParticlePreset(particleShapeIdx, particleBehaviorIdx);
        }
    }
    // 'R': Reset current scene objects
    else if (c == 'r' || c == 'R') {
        manualControlActive = false;
        clothTearMode = false;
        resetSceneObjects();
        Serial.println("Scene Reset!");
        Serial0.println("Scene Reset!");
    }
}

void handleSerialInput()
{
    while (Serial.available()) {
        processCommandChar((char)Serial.read());
    }
    while (Serial0.available()) {
        processCommandChar((char)Serial0.read());
    }
}

void setup()
{
    Serial.begin(115200);
    Serial0.begin(115200);
    randomSeed(analogRead(0));
    delay(300);

    if (!initCH422G()) {
        Serial.println("CH422G INIT FAILED");
        Serial0.println("CH422G INIT FAILED");
        return;
    }
    if (!engine.begin(PixelFormat::RGB565)) {
        Serial.println("RGBGameEngine INIT FAILED");
        Serial0.println("RGBGameEngine INIT FAILED");
        return;
    }

    resetSceneObjects();
    sceneStartTime = millis();
    const char *readyMsg = "RGBGameEngine Interactive Suite Ready on Serial & Serial0! Use control.py (COM8) or Serial Monitor.";
    Serial.println(readyMsg);
    Serial0.println(readyMsg);
}

void loop()
{
    static uint32_t lastUs = micros();
    static float smoothDt  = 0.016f;
    uint32_t nowUs = micros();
    float rawDt = (nowUs - lastUs) / 1000000.0f;
    lastUs = nowUs;
    rawDt = constrain(rawDt, 0.004f, 0.035f);
    // Exponential moving-average frame-time filter for butter-smooth motion!
    smoothDt = smoothDt * 0.80f + rawDt * 0.20f;
    float dt = smoothDt;

    handleSerialInput();

    // Smooth continuous cursor movement when holding WASD keys in control.py
    if (manualControlActive && (velX != 0.0f || velY != 0.0f)) {
        cursorX = constrain(cursorX + velX * dt, 20.0f, 780.0f);
        cursorY = constrain(cursorY + velY * dt, 55.0f, 465.0f);
    }

    if (autoCycleScenes && (millis() - sceneStartTime > 7000)) {
        sceneStartTime = millis();
        currentScene = (currentScene % 9) + 1;
    }

    float t = millis() * 0.001f;

    switch (currentScene) {
        case 1: renderScene1_Shapes(t); break;
        case 2: renderScene2_ColorPhysics(t); break;
        case 3: renderScene3_Transparency(t); break;
        case 4: renderScene4_LightingAndShadows(t); break;
        case 5: renderScene5_ShatterLab(t, dt); break;
        case 6: renderScene6_CushionCompression(t, dt); break;
        case 7: renderScene7_ClothAndScarf(t, dt); break;
        case 8: renderScene8_FullGamePhysics(t, dt); break;
        case 9: renderScene9_ParticleLab(t, dt); break;
        default: renderScene1_Shapes(t); break;
    }

    engine.show();
}
