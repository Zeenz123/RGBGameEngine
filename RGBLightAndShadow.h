#ifndef RGB_LIGHT_AND_SHADOW_H
#define RGB_LIGHT_AND_SHADOW_H

#include "RGBGameEngine.h"
#include <math.h>

// ============================================================================
// 1. LIGHT SOURCE TYPES & SCENE OCCLUDERS
// ============================================================================

enum class LightType : uint8_t
{
    POINT_RADIAL = 0, // 360-degree radial point light
    SPOT_CONE,        // Directional cone / flashlight beam
    LINE_SEGMENT,     // Glowing neon tube / laser beam segment
    RECT_AREA,        // Rectangular area emitter
    CIRCLE_AREA       // Circular ring/disc area emitter
};

struct LightSource2D
{
    LightType type;
    float x, y;           // Primary position (or start of line / center of shape)
    float x2, y2;         // End of line segment, or width/height of RECT_AREA
    float radius;         // Falloff radius in pixels
    float intensity;      // 0.0f (off) to 2.0f+ (overdriven bright core)
    float dirAngleDeg;    // Direction for SPOT_CONE
    float coneHalfDeg;    // Half-angle of spotlight cone
    uint16_t color565;    // Light RGB565 tint
    bool enabled;

    static LightSource2D createPoint(float px, float py, float rad, uint16_t col, float inten = 1.0f) {
        return { LightType::POINT_RADIAL, px, py, 0, 0, rad, inten, 0, 180, col, true };
    }
    static LightSource2D createSpot(float px, float py, float rad, float dirDeg, float halfConeDeg, uint16_t col, float inten = 1.2f) {
        return { LightType::SPOT_CONE, px, py, 0, 0, rad, inten, dirDeg, halfConeDeg, col, true };
    }
    static LightSource2D createLine(float x0, float y0, float x1, float y1, float glowRad, uint16_t col, float inten = 1.0f) {
        return { LightType::LINE_SEGMENT, x0, y0, x1, y1, glowRad, inten, 0, 0, col, true };
    }
    static LightSource2D createRectArea(float cx, float cy, float w, float h, float glowRad, uint16_t col, float inten = 1.0f) {
        return { LightType::RECT_AREA, cx, cy, w, h, glowRad, inten, 0, 0, col, true };
    }
};

enum class OccluderShape : uint8_t
{
    CIRCLE = 0,
    BOX
};

struct Occluder2D
{
    OccluderShape shape;
    float x, y;     // Center for CIRCLE, top-left for BOX
    float w, h;     // w = radius for CIRCLE; w, h = dimensions for BOX

    static Occluder2D makeCircle(float cx, float cy, float r) {
        return { OccluderShape::CIRCLE, cx, cy, r, r };
    }
    static Occluder2D makeBox(float bx, float by, float bw, float bh) {
        return { OccluderShape::BOX, bx, by, bw, bh };
    }
};

// ============================================================================
// 2. ULTRA-FAST SINGLE-PASS 2D LIGHTING & SHADOW ENGINE (ZERO OVERDRAW)
// ============================================================================

class LightSystem2D
{
public:
    // Fast inline saturating RGB565 addition with 5-bit intensity scale (scale32 in 0..32)
    static inline uint16_t fastAddLight565(uint16_t dst, uint32_t lightR5, uint32_t lightG6, uint32_t lightB5, uint32_t scale32)
    {
        uint32_t r = ((dst >> 11) & 0x1F) + ((lightR5 * scale32) >> 5);
        uint32_t g = ((dst >> 5)  & 0x3F) + ((lightG6 * scale32) >> 5);
        uint32_t b = ( dst        & 0x1F) + ((lightB5 * scale32) >> 5);
        if (r > 31) r = 31;
        if (g > 63) g = 63;
        if (b > 31) b = 31;
        return (uint16_t)((r << 11) | (g << 5) | b);
    }

    // ------------------------------------------------------------------------
    // Single-Pass Point Light (Touches each pixel ONCE, 0 sqrtf per pixel!)
    // ------------------------------------------------------------------------
    static void drawPointLight(RGBGameEngine &eng, float lx, float ly, float radius, uint16_t color565, float intensity = 1.0f)
    {
        if (radius <= 1.0f || intensity <= 0.01f) return;
        uint16_t *fb = eng.getDrawBuffer();
        if (!fb) return;

        int16_t r = (int16_t)radius;
        int16_t cx = (int16_t)lx, cy = (int16_t)ly;
        int32_t r2 = (int32_t)r * r;
        if (r2 <= 0) return;

        uint32_t lr5 = (color565 >> 11) & 0x1F;
        uint32_t lg6 = (color565 >> 5)  & 0x3F;
        uint32_t lb5 =  color565        & 0x1F;

        // Pre-scale intensity into 0..48 fixed point
        uint32_t maxScale = (uint32_t)(intensity * 32.0f);
        if (maxScale > 64) maxScale = 64;

        int16_t yStart = max((int16_t)0, (int16_t)(cy - r));
        int16_t yEnd   = min((int16_t)(eng.height() - 1), (int16_t)(cy + r));
        int16_t scrW   = eng.width();

        for (int16_t sy = yStart; sy <= yEnd; sy++) {
            int32_t dy = sy - cy;
            int32_t dy2 = dy * dy;
            int16_t span = (int16_t)sqrtf((float)(r2 - dy2));

            int16_t xStart = max((int16_t)0, (int16_t)(cx - span));
            int16_t xEnd   = min((int16_t)(scrW - 1), (int16_t)(cx + span));
            uint16_t *row  = &fb[sy * scrW];

            for (int16_t sx = xStart; sx <= xEnd; sx++) {
                int32_t dx = sx - cx;
                int32_t d2 = dx * dx + dy2;
                // Pure integer quadratic falloff: (r2 - d2)^2 / r2^2
                uint32_t rem = (uint32_t)(r2 - d2);
                uint32_t lin32 = (rem * 32U) / (uint32_t)r2;       // 0..32 linear
                uint32_t quad32 = (lin32 * lin32 * maxScale) >> 10; // Smooth quadratic falloff!
                if (quad32 > 0) {
                    row[sx] = fastAddLight565(row[sx], lr5, lg6, lb5, quad32);
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // Render a Directional Spot / Cone Light (Fast Single-Triangle-per-Sector)
    // ------------------------------------------------------------------------
    static void drawSpotLight(RGBGameEngine &eng, float lx, float ly, float radius, float dirDeg, float halfConeDeg, uint16_t color565, float intensity = 1.0f)
    {
        if (radius <= 2.0f || intensity <= 0.01f) return;
        float a1 = (dirDeg - halfConeDeg) * 0.0174532925f;
        float a2 = (dirDeg + halfConeDeg) * 0.0174532925f;
        int16_t x1 = (int16_t)(lx + cosf(a1) * radius);
        int16_t y1 = (int16_t)(ly + sinf(a1) * radius);
        int16_t x2 = (int16_t)(lx + cosf(a2) * radius);
        int16_t y2 = (int16_t)(ly + sinf(a2) * radius);

        uint8_t alpha = (uint8_t)min(220.0f, 95.0f * intensity);
        eng.fillTriangleAlpha((int16_t)lx, (int16_t)ly, x1, y1, x2, y2, color565, alpha);
    }

    // ------------------------------------------------------------------------
    // Render a Photorealistic Glowing Stick / Lightsaber / Neon Tube Light
    // Uses Single-Pass Capsule Signed Distance Field (SDF) Additive Bloom!
    // ------------------------------------------------------------------------
    static void drawLineLight(RGBGameEngine &eng, float x0, float y0, float x1, float y1, float glowRadius, uint16_t color565, float intensity = 1.0f)
    {
        if (glowRadius <= 1.0f || intensity <= 0.01f) return;
        uint16_t *fb = eng.getDrawBuffer();
        if (!fb) return;

        int16_t r = (int16_t)glowRadius;
        int32_t r2 = (int32_t)r * r;
        int32_t coreR2 = max((int32_t)4, r2 / 28); // White-hot inner filament radius^2

        int16_t minX = max((int16_t)0, (int16_t)(min(x0, x1) - r));
        int16_t maxX = min((int16_t)(eng.width() - 1), (int16_t)(max(x0, x1) + r));
        int16_t minY = max((int16_t)0, (int16_t)(min(y0, y1) - r));
        int16_t maxY = min((int16_t)(eng.height() - 1), (int16_t)(max(y0, y1) + r));

        int32_t vx = (int32_t)(x1 - x0);
        int32_t vy = (int32_t)(y1 - y0);
        int32_t lenSq = vx * vx + vy * vy;
        if (lenSq < 1) lenSq = 1;

        uint32_t lr5 = (color565 >> 11) & 0x1F;
        uint32_t lg6 = (color565 >> 5)  & 0x3F;
        uint32_t lb5 =  color565        & 0x1F;
        uint32_t maxScale = min((uint32_t)72, (uint32_t)(intensity * 36.0f));
        int16_t scrW = eng.width();

        int32_t ix0 = (int32_t)x0, iy0 = (int32_t)y0;

        for (int16_t sy = minY; sy <= maxY; sy++) {
            uint16_t *row = &fb[sy * scrW];
            int32_t py = sy - iy0;

            for (int16_t sx = minX; sx <= maxX; sx++) {
                int32_t px = sx - ix0;
                // Project pixel onto line segment [0..256] fixed-point
                int32_t dot = px * vx + py * vy;
                int32_t t256 = (dot << 8) / lenSq;
                if (t256 < 0)   t256 = 0;
                if (t256 > 256) t256 = 256;

                int32_t cx = ix0 + ((vx * t256) >> 8);
                int32_t cy = iy0 + ((vy * t256) >> 8);
                int32_t dx = sx - cx;
                int32_t dy = sy - cy;
                int32_t d2 = dx * dx + dy * dy;

                if (d2 < r2) {
                    if (d2 <= coreR2) {
                        // Blinding white-hot center plasma filament tinted with light color
                        row[sx] = Color::blend565(Colors565::White, color565, 210);
                    } else {
                        // Smooth quadratic atmospheric bloom aura
                        uint32_t rem = (uint32_t)(r2 - d2);
                        uint32_t lin32 = (rem * 32U) / (uint32_t)r2;
                        uint32_t quad32 = (lin32 * lin32 * maxScale) >> 10;
                        if (quad32 > 0) {
                            row[sx] = fastAddLight565(row[sx], lr5, lg6, lb5, quad32);
                        }
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // Render any Shape (Rect) as an Area Light Emitter (Fast Border Halo)
    // ------------------------------------------------------------------------
    static void drawRectAreaLight(RGBGameEngine &eng, int16_t x, int16_t y, int16_t w, int16_t h, int16_t glowRadius, uint16_t color565, float intensity = 1.0f)
    {
        uint8_t aOuter = (uint8_t)min(255.0f, 55.0f * intensity);
        uint8_t aInner = (uint8_t)min(255.0f, 110.0f * intensity);
        int16_t rHalf  = glowRadius / 2;

        eng.fillRoundRectAlpha(x - glowRadius, y - glowRadius, w + glowRadius * 2, h + glowRadius * 2, glowRadius, color565, aOuter);
        eng.fillRoundRectAlpha(x - rHalf,      y - rHalf,      w + rHalf * 2,      h + rHalf * 2,      rHalf,      color565, aInner);
        eng.fillRect(x, y, w, h, Color::blend565(Colors565::White, color565, 160));
    }

    // ------------------------------------------------------------------------
    // Fast Analytic Projected Shadow (Umbra + Soft Penumbra) for a Circle
    // Shadow darkness scales dynamically with lightIntensity & distance!
    // ------------------------------------------------------------------------
    static void castCircleShadow(RGBGameEngine &eng, float lightX, float lightY, float cx, float cy, float radius, float shadowLength = 280.0f, uint8_t baseDarkness = 140, float lightIntensity = 1.0f)
    {
        if (lightIntensity <= 0.02f || baseDarkness == 0) return;
        float dx = cx - lightX;
        float dy = cy - lightY;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < radius + 2.0f) return;

        // Distance attenuation + lightIntensity scaling:
        // Brighter light & closer light => deeper umbra shadow; dimmer/farther => softer shadow!
        float distFactor = constrain(1.25f - (dist / (shadowLength * 2.2f)), 0.25f, 1.15f);
        uint8_t umbraAlpha = (uint8_t)constrain((int)(baseDarkness * lightIntensity * distFactor), 8, 242);
        uint8_t penumbraAlpha = umbraAlpha >> 1;

        float invDist = 1.0f / dist;
        float ux = dx * invDist;
        float uy = dy * invDist;
        float px = -uy;
        float py =  ux;

        int16_t t1x = (int16_t)(cx + px * radius);
        int16_t t1y = (int16_t)(cy + py * radius);
        int16_t t2x = (int16_t)(cx - px * radius);
        int16_t t2y = (int16_t)(cy - py * radius);

        float spread = 1.0f + (shadowLength / max(70.0f, dist));
        float farCx = cx + ux * shadowLength;
        float farCy = cy + uy * shadowLength;

        int16_t f1x = (int16_t)(farCx + px * radius * spread);
        int16_t f1y = (int16_t)(farCy + py * radius * spread);
        int16_t f2x = (int16_t)(farCx - px * radius * spread);
        int16_t f2y = (int16_t)(farCy - py * radius * spread);

        // Soft penumbra wings (zero overlap with umbra!)
        int16_t p1x = (int16_t)(farCx + px * radius * (spread * 1.28f));
        int16_t p1y = (int16_t)(farCy + py * radius * (spread * 1.28f));
        int16_t p2x = (int16_t)(farCx - px * radius * (spread * 1.28f));
        int16_t p2y = (int16_t)(farCy - py * radius * (spread * 1.28f));

        eng.fillTriangleAlpha(t1x, t1y, p1x, p1y, f1x, f1y, 0x0000, penumbraAlpha);
        eng.fillTriangleAlpha(t2x, t2y, f2x, f2y, p2x, p2y, 0x0000, penumbraAlpha);

        // Core umbra quad (2 triangles, zero overdraw)
        eng.fillTriangleAlpha(t1x, t1y, f1x, f1y, f2x, f2y, 0x0000, umbraAlpha);
        eng.fillTriangleAlpha(t1x, t1y, f2x, f2y, t2x, t2y, 0x0000, umbraAlpha);
    }

    // ------------------------------------------------------------------------
    // Fast Analytic Projected Shadow for a Rectangular Box
    // Shadow darkness scales dynamically with lightIntensity & distance!
    // ------------------------------------------------------------------------
    static void castBoxShadow(RGBGameEngine &eng, float lightX, float lightY, float bx, float by, float bw, float bh, float shadowLength = 280.0f, uint8_t baseDarkness = 140, float lightIntensity = 1.0f)
    {
        if (lightIntensity <= 0.02f || baseDarkness == 0) return;
        float centerDist = Vec2((bx + bw * 0.5f) - lightX, (by + bh * 0.5f) - lightY).length();
        float distFactor = constrain(1.25f - (centerDist / (shadowLength * 2.2f)), 0.25f, 1.15f);
        uint8_t umbraAlpha = (uint8_t)constrain((int)(baseDarkness * lightIntensity * distFactor), 8, 242);

        Vec2 corners[4] = {
            {bx, by}, {bx + bw, by}, {bx + bw, by + bh}, {bx, by + bh}
        };

        for (int i = 0; i < 4; i++) {
            Vec2 a = corners[i];
            Vec2 b = corners[(i + 1) & 3];
            Vec2 edge = b - a;
            Vec2 normal(edge.y, -edge.x);
            Vec2 mid = (a + b) * 0.5f;
            Vec2 toMid = mid - Vec2(lightX, lightY);

            if (normal.dot(toMid) > 0.0f) {
                Vec2 dirA = (a - Vec2(lightX, lightY)).normalized();
                Vec2 dirB = (b - Vec2(lightX, lightY)).normalized();
                Vec2 farA = a + dirA * shadowLength;
                Vec2 farB = b + dirB * shadowLength;

                eng.fillTriangleAlpha((int16_t)a.x, (int16_t)a.y, (int16_t)b.x, (int16_t)b.y, (int16_t)farB.x, (int16_t)farB.y, 0x0000, umbraAlpha);
                eng.fillTriangleAlpha((int16_t)a.x, (int16_t)a.y, (int16_t)farB.x, (int16_t)farB.y, (int16_t)farA.x, (int16_t)farA.y, 0x0000, umbraAlpha);
            }
        }
    }

    // ------------------------------------------------------------------------
    // 2D Ray-Casted Visibility / Light Fan (True 2D Ray Casting against Occluders)
    // ------------------------------------------------------------------------
    static float raycastAgainstOccluders(float ox, float oy, float dx, float dy, float maxDist, const Occluder2D *occluders, int count)
    {
        float closest = maxDist;

        for (int i = 0; i < count; i++) {
            const Occluder2D &occ = occluders[i];
            if (occ.shape == OccluderShape::CIRCLE) {
                float fx = ox - occ.x;
                float fy = oy - occ.y;
                float b = 2.0f * (fx * dx + fy * dy);
                float c = (fx * fx + fy * fy) - occ.w * occ.w;
                float disc = b * b - 4.0f * c;
                if (disc >= 0.0f) {
                    float t = (-b - sqrtf(disc)) * 0.5f;
                    if (t > 0.0f && t < closest) closest = t;
                }
            } else {
                float invDx = (fabsf(dx) > 0.0001f) ? (1.0f / dx) : 1e6f;
                float invDy = (fabsf(dy) > 0.0001f) ? (1.0f / dy) : 1e6f;
                float t1 = (occ.x - ox) * invDx;
                float t2 = (occ.x + occ.w - ox) * invDx;
                float t3 = (occ.y - oy) * invDy;
                float t4 = (occ.y + occ.h - oy) * invDy;

                float tmin = max(min(t1, t2), min(t3, t4));
                float tmax = min(max(t1, t2), max(t3, t4));
                if (tmax >= 0.0f && tmin <= tmax && tmin > 0.0f && tmin < closest) {
                    closest = tmin;
                }
            }
        }
        return closest;
    }

    static void drawRaytracedLight(RGBGameEngine &eng, float lx, float ly, float maxRadius, uint16_t lightColor, uint8_t alpha, const Occluder2D *occluders, int occCount, int numRays = 90)
    {
        if (numRays < 16) numRays = 16;
        if (numRays > 180) numRays = 180;

        float stepAngle = 6.2831853f / numRays;
        int16_t prevX = 0, prevY = 0;

        for (int i = 0; i <= numRays; i++) {
            float a = i * stepAngle;
            float dx = cosf(a), dy = sinf(a);
            float hitDist = raycastAgainstOccluders(lx, ly, dx, dy, maxRadius, occluders, occCount);
            int16_t hx = (int16_t)(lx + dx * hitDist);
            int16_t hy = (int16_t)(ly + dy * hitDist);

            if (i > 0) {
                eng.fillTriangleAlpha((int16_t)lx, (int16_t)ly, prevX, prevY, hx, hy, lightColor, alpha);
            }
            prevX = hx; prevY = hy;
        }
    }

    // ------------------------------------------------------------------------
    // Procedural Jagged Lightning Bolt & Electric Arc Generator
    // ------------------------------------------------------------------------
    static void drawLightningBolt(RGBGameEngine &eng, float x0, float y0, float x1, float y1, float sway = 36.0f, uint16_t glowColor = Colors565::ElectricBlue, int generations = 4)
    {
        Vec2 pts[33];
        int count = 1 << generations;
        if (count > 32) count = 32;

        pts[0] = Vec2(x0, y0);
        pts[count] = Vec2(x1, y1);

        int step = count;
        float curSway = sway;
        while (step > 1) {
            int half = step >> 1;
            for (int i = 0; i < count; i += step) {
                Vec2 mid = (pts[i] + pts[i + step]) * 0.5f;
                Vec2 dir = (pts[i + step] - pts[i]).normalized().perp();
                float offset = ((float)(random(-100, 101)) / 100.0f) * curSway;
                pts[i + half] = mid + dir * offset;
            }
            curSway *= 0.52f;
            step = half;
        }

        for (int i = 0; i < count; i++) {
            int16_t ax = (int16_t)pts[i].x,     ay = (int16_t)pts[i].y;
            int16_t bx = (int16_t)pts[i + 1].x, by = (int16_t)pts[i + 1].y;
            eng.drawThickLine(ax, ay, bx, by, 5, Color::scaleBrightness565(glowColor, 110), true);
            eng.drawThickLine(ax, ay, bx, by, 2, glowColor, true);
            eng.drawLine(ax, ay, bx, by, Colors565::White);

            if (generations >= 3 && (i == count / 3 || i == (2 * count) / 3)) {
                Vec2 branchEnd = pts[i] + (pts[i + 1] - pts[i]) * 2.8f + Vec2(random(-35, 36), random(15, 45));
                drawLightningBolt(eng, pts[i].x, pts[i].y, branchEnd.x, branchEnd.y, sway * 0.35f, glowColor, 2);
            }
        }
    }
};

#endif
