#ifndef RGB_GAME_ENGINE_H
#define RGB_GAME_ENGINE_H

#include "waveshare_lcd_port.h"
#include "Color.h"
#include "PixelFormat.h"
#include "Point.h"

class RGBGameEngine {
public:
  RGBGameEngine();

  static void refreshCallback();
  void onRefreshFinish();

  bool begin(PixelFormat format = PixelFormat::RGB565);
  bool playVideoFrame(const uint8_t *data);

  uint16_t *getFrameBuffer();
  uint16_t *getDrawBuffer();
  uint16_t getPixel(int x, int y) const;

  int width() const;
  int height() const;

  void clear(uint16_t color);
  void clear(Color color);
  void show();

  // ==================================================
  // 1. PIXELS, TRANSPARENCY & BLEND MODES
  // ==================================================
  void drawPixel(int x, int y, uint16_t color);
  void drawPixel(int x, int y, Color color);
  void drawPixelAlpha(int x, int y, uint16_t color, uint8_t alpha);
  void drawPixelBlend(int x, int y, uint16_t color, uint8_t alpha, BlendMode mode);

  // ==================================================
  // 2. LINES (FAST, THICK, DASHED, ANTI-ALIASED)
  // ==================================================
  void drawFastHLine(int16_t x, int16_t y, int16_t width, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t width, Color color);
  void drawFastHLineAlpha(int16_t x, int16_t y, int16_t width, uint16_t color, uint8_t alpha);
  void drawFastHLineBlend(int16_t x, int16_t y, int16_t width, uint16_t color, uint8_t alpha, BlendMode mode);

  void drawFastVLine(int16_t x, int16_t y, int16_t height, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t height, Color color);

  void drawLine(int x0, int y0, int x1, int y1, uint16_t color);
  void drawLine(int x0, int y0, int x1, int y1, Color color);
  void drawLineAlpha(int x0, int y0, int x1, int y1, uint16_t color, uint8_t alpha);

  void drawThickLine(int x0, int y0, int x1, int y1, int16_t thickness, uint16_t color, bool roundCaps = true);
  void drawDashedLine(int x0, int y0, int x1, int y1, int16_t dashLen, int16_t gapLen, uint16_t color);
  void drawAntiAliasedLine(int x0, int y0, int x1, int y1, uint16_t color);

  // ==================================================
  // 3. RECTANGLES, ROUNDED RECTS & QUADS
  // ==================================================
  void drawRect(int x, int y, int width, int height, uint16_t color);
  void drawRect(int x, int y, int width, int height, Color color);
  void drawThickRect(int x, int y, int width, int height, int16_t border, uint16_t color);

  void fillRect(int x, int y, int width, int height, uint16_t color);
  void fillRect(int x, int y, int width, int height, Color color);
  void fillRectAlpha(int x, int y, int width, int height, uint16_t color, uint8_t alpha);
  void fillRectBlend(int x, int y, int width, int height, uint16_t color, uint8_t alpha, BlendMode mode);

  void drawRoundRect(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color);
  void drawRoundRect(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, Color color);
  void fillRoundRect(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color);
  void fillRoundRect(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, Color color);
  void fillRoundRectAlpha(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color, uint8_t alpha);
  void fillRoundRectBlend(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color, uint8_t alpha, BlendMode mode);
  void fillVerticalAlphaGradientRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color, uint8_t topAlpha, uint8_t bottomAlpha);
  void drawGlassLensCircle(int16_t cx, int16_t cy, int16_t radius, uint16_t tintColor, uint8_t alpha, float refractionStrength = 0.25f);

  void drawQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color);
  void fillQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color);
  void fillDiamond(int16_t centerX, int16_t centerY, int16_t halfW, int16_t halfH, uint16_t color);

  // ==================================================
  // 4. POLYLINES, POLYGONS, STARS & N-GONS
  // ==================================================
  void drawPolyline(const Point *points, uint16_t pointCount, uint16_t color);
  void drawPolyline(const Point *points, uint16_t pointCount, Color color);

  void drawPolygon(const Point *points, uint16_t pointCount, uint16_t color);
  void drawPolygon(const Point *points, uint16_t pointCount, Color color);
  void fillPolygon(const Point *points, uint16_t pointCount, uint16_t color);
  void fillPolygonAlpha(const Point *points, uint16_t pointCount, uint16_t color, uint8_t alpha);

  void drawRegularPolygon(int16_t cx, int16_t cy, uint8_t sides, int16_t radius, float rotationDeg, uint16_t color);
  void fillRegularPolygon(int16_t cx, int16_t cy, uint8_t sides, int16_t radius, float rotationDeg, uint16_t color);

  void drawStar(int16_t cx, int16_t cy, uint8_t points, int16_t outerRadius, int16_t innerRadius, float rotationDeg, uint16_t color);
  void fillStar(int16_t cx, int16_t cy, uint8_t points, int16_t outerRadius, int16_t innerRadius, float rotationDeg, uint16_t color);

  // ==================================================
  // 5. TRIANGLES (SOLID, ALPHA & GOURAUD SHADED)
  // ==================================================
  void drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color);
  void drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Color color);

  void fillTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color);
  void fillTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Color color);
  void fillTriangleAlpha(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color, uint8_t alpha);
  void fillTriangleGouraud(int16_t x1, int16_t y1, uint16_t c1, int16_t x2, int16_t y2, uint16_t c2, int16_t x3, int16_t y3, uint16_t c3);

  // ==================================================
  // 6. CIRCLES, RINGS, ELLIPSES, ARCS & CAPSULES
  // ==================================================
  void drawCircle(int16_t x, int16_t y, int16_t radius, uint16_t color);
  void drawCircle(int16_t x, int16_t y, int16_t radius, Color color);
  void fillCircle(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color);
  void fillCircle(int16_t centerX, int16_t centerY, int16_t radius, Color color);
  void fillCircleAlpha(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color, uint8_t alpha);
  void fillCircleBlend(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color, uint8_t alpha, BlendMode mode);

  void drawRing(int16_t cx, int16_t cy, int16_t innerRadius, int16_t outerRadius, uint16_t color);
  void fillRing(int16_t cx, int16_t cy, int16_t innerRadius, int16_t outerRadius, uint16_t color);

  void drawEllipse(int16_t centerX, int16_t centerY, int16_t radiusX, int16_t radiusY, uint16_t color);
  void drawEllipse(int16_t centerX, int16_t centerY, int16_t radiusX, int16_t radiusY, Color color);
  void fillEllipse(int16_t centerX, int16_t centerY, int16_t radiusX, int16_t radiusY, uint16_t color);
  void fillEllipse(int16_t centerX, int16_t centerY, int16_t radiusX, int16_t radiusY, Color color);

  void drawArc(int16_t centerX, int16_t centerY, int16_t radius, float startAngleDeg, float endAngleDeg, uint16_t color);
  void drawArc(int16_t centerX, int16_t centerY, int16_t radius, float startAngleDeg, float endAngleDeg, Color color);
  void fillArc(int16_t centerX, int16_t centerY, int16_t radius, float startAngleDeg, float endAngleDeg, uint16_t color);
  void fillArc(int16_t centerX, int16_t centerY, int16_t radius, float startAngleDeg, float endAngleDeg, Color color);
  void fillArcRing(int16_t centerX, int16_t centerY, int16_t innerRadius, int16_t outerRadius, float startAngleDeg, float endAngleDeg, uint16_t color);

  void drawCapsule(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t radius, uint16_t color);
  void fillCapsule(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t radius, uint16_t color);

  // ==================================================
  // 7. CURVES, SPLINES, ARROWS & 2.5D WIREFRAMES
  // ==================================================
  void drawQuadBezier(int16_t x0, int16_t y0, int16_t cx, int16_t cy, int16_t x1, int16_t y1, uint16_t color, int16_t thickness = 1);
  void drawCubicBezier(int16_t x0, int16_t y0, int16_t cx0, int16_t cy0, int16_t cx1, int16_t cy1, int16_t x1, int16_t y1, uint16_t color, int16_t thickness = 1);
  void drawCatmullRomSpline(const Point *points, uint16_t count, uint16_t color, int16_t thickness = 1);

  void drawArrow(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t headSize, uint16_t color, int16_t thickness = 2);
  void drawCrosshair(int16_t cx, int16_t cy, int16_t radius, uint16_t color);

  void drawCube3D(int16_t centerX, int16_t centerY, float size, float rotX, float rotY, float rotZ, uint16_t color);
  void drawPyramid3D(int16_t centerX, int16_t centerY, float size, float rotX, float rotY, float rotZ, uint16_t color);
  void drawSphereGrid3D(int16_t centerX, int16_t centerY, float radius, float rotX, float rotY, uint16_t color);

  // ==================================================
  // 8. GRADIENTS (WITH 4x4 BAYER ORDERED DITHERING)
  // ==================================================
  void fillLinearGradientRect(int16_t x, int16_t y, int16_t w, int16_t h, Color startColor, Color endColor, bool vertical = true, bool dither = true);
  void fillDiagonalGradientRect(int16_t x, int16_t y, int16_t w, int16_t h, Color topLeft, Color bottomRight, bool dither = true);
  void fillRadialGradientCircle(int16_t cx, int16_t cy, int16_t radius, Color innerColor, Color outerColor, bool dither = true);
  void drawDropShadowRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, int16_t offsetX, int16_t offsetY, int16_t blurPasses, uint8_t shadowAlpha = 140);
  void drawDropShadowCircle(int16_t cx, int16_t cy, int16_t radius, int16_t offsetX, int16_t offsetY, int16_t blurPasses, uint8_t shadowAlpha = 140);

  // ==================================================
  // 9. SPRITE ENGINE (FLIP, SCALE, ROTATE, SHADOW, SQUASH)
  // ==================================================
  void drawSprite(const uint16_t *pixels, int16_t w, int16_t h, int16_t x, int16_t y, uint16_t colorKey = 0x0000, bool flipX = false, bool flipY = false);
  void drawSpriteAlpha(const uint16_t *pixels, int16_t w, int16_t h, int16_t x, int16_t y, uint8_t alpha, uint16_t colorKey = 0x0000, bool flipX = false);
  void drawSpriteTinted(const uint16_t *pixels, int16_t w, int16_t h, int16_t x, int16_t y, uint16_t tintColor, uint8_t alpha, uint16_t colorKey = 0x0000);
  void drawSpriteScaled(const uint16_t *pixels, int16_t srcW, int16_t srcH, int16_t dstX, int16_t dstY, int16_t dstW, int16_t dstH, uint16_t colorKey = 0x0000, bool flipX = false);
  void drawSpriteRotated(const uint16_t *pixels, int16_t w, int16_t h, int16_t centerX, int16_t centerY, float angleDeg, uint16_t colorKey = 0x0000);
  void drawSpriteShadow(const uint16_t *pixels, int16_t w, int16_t h, int16_t footX, int16_t footY, float skewX = 0.45f, float scaleY = 0.35f, uint8_t shadowAlpha = 120, uint16_t colorKey = 0x0000);

  // ==================================================
  // 10. BUILT-IN FONT & TEXT RENDERER
  // ==================================================
  void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint8_t scale = 1);
  void drawText(int16_t x, int16_t y, const char *str, uint16_t color, uint8_t scale = 2);
  void drawTextShadow(int16_t x, int16_t y, const char *str, uint16_t textColor, uint16_t shadowColor, uint8_t scale = 2, int16_t shadowOffset = 2);
  void drawTextOutline(int16_t x, int16_t y, const char *str, uint16_t textColor, uint16_t outlineColor, uint8_t scale = 2);

  // ==================================================
  // 11. SCREEN POST-PROCESSING, SKY DOME & TERRAIN
  // ==================================================
  void applyScreenTint(uint16_t tint565, uint8_t alpha = 160);
  void applyRedGameOverFilter();
  void applyCRTScanlines(uint8_t darkness = 170, int16_t spacing = 2);
  void applyVignette(int16_t borderThickness = 90, uint8_t maxDarkness = 180);
  void drawShockwaveRing(int16_t cx, int16_t cy, int16_t radius, int16_t bandWidth, float strength = 8.0f);
  void drawIrisWipe(int16_t focusX, int16_t focusY, int16_t openRadius, uint16_t maskColor = 0x0000);

  void drawParallaxHorizonStrip(const int16_t *heightMap, int count, int16_t segmentWidth, float scrollX, int16_t baseY, uint16_t litColor, uint16_t shadowColor, uint16_t snowCapColor = Colors565::White, int16_t snowThreshold = 125);
  void drawAtmosphericSkyDome(float timeOfDay01, int16_t horizonY, bool drawStars = true);

private:
  static RGBGameEngine *instance;

  uint16_t *frontBuffer;
  uint16_t *backBuffer;
  uint16_t *frameBuffer;

  volatile bool refreshFinished;
  volatile bool swapPending;

  esp_panel::drivers::LCD *lcd;
  PixelFormat pixelFormat;

  struct DisplayInfo {
    uint16_t width;
    uint16_t height;
    uint8_t bitsPerPixel;
  };
  DisplayInfo display;

  void drawCirclePoints(int16_t centerX, int16_t centerY, int16_t x, int16_t y, uint16_t color);
  void drawCircleCorner(int16_t centerX, int16_t centerY, int16_t radius, uint8_t cornerMask, uint16_t color);
  void fillCircleCorner(int16_t centerX, int16_t centerY, int16_t radius, uint8_t cornerMask, int16_t delta, uint16_t color);
  void drawEllipsePoints(int16_t centerX, int16_t centerY, int16_t x, int16_t y, uint16_t color);
};

// Include Advanced Subsystems (Lighting, Ray-Casted Shadows, Physics, Shatter, SoftBody, Cloth, Water, Particles, UI)
#include "RGBLightAndShadow.h"
#include "RGBPhysics2D.h"

#endif
