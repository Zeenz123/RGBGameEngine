#ifndef RGB_PHYSICS_2D_H
#define RGB_PHYSICS_2D_H

#include "RGBGameEngine.h"
#include <math.h>

// ============================================================================
// 1. EASING & TWEENING CURVES
// ============================================================================
namespace Easing
{
    inline float linear(float t)      { return t; }
    inline float inQuad(float t)      { return t * t; }
    inline float outQuad(float t)     { return t * (2.0f - t); }
    inline float inOutQuad(float t)   { return (t < 0.5f) ? (2.0f * t * t) : (-1.0f + (4.0f - 2.0f * t) * t); }
    inline float outCubic(float t)    { float u = t - 1.0f; return u * u * u + 1.0f; }
    inline float outBack(float t)     { const float s = 1.70158f; float u = t - 1.0f; return u * u * ((s + 1.0f) * u + s) + 1.0f; }
    inline float outElastic(float t)  {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * 2.0943951f) + 1.0f;
    }
    inline float outBounce(float t)   {
        const float n1 = 7.5625f, d1 = 2.75f;
        if (t < 1.0f / d1) return n1 * t * t;
        if (t < 2.0f / d1) { t -= 1.5f / d1; return n1 * t * t + 0.75f; }
        if (t < 2.5f / d1) { t -= 2.25f / d1; return n1 * t * t + 0.9375f; }
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

// ============================================================================
// 2. RIGID BODY KINEMATICS, COLLISION MANIFOLDS & SPRINGS
// ============================================================================
struct RigidBody2D
{
    Vec2 pos;
    Vec2 vel;
    Vec2 forceAccum;
    float radius;
    float mass;
    float invMass;
    float restitution; // Bounciness: 0.0 = inelastic, 0.9 = super bouncy
    float drag;        // Air resistance
    float friction;    // Surface contact friction
    float angleRad;
    float angularVel;
    uint16_t color;

    void init(float px, float py, float r, float m, float bounce = 0.78f, uint16_t col = Colors565::Crimson) {
        pos = Vec2(px, py);
        vel = Vec2(0, 0);
        forceAccum = Vec2(0, 0);
        radius = r;
        mass = m;
        invMass = (m > 0.0001f) ? (1.0f / m) : 0.0f;
        restitution = bounce;
        drag = 0.995f;
        friction = 0.98f;
        angleRad = 0.0f;
        angularVel = 0.0f;
        color = col;
    }

    void applyForce(const Vec2 &f)   { forceAccum += f; }
    void applyImpulse(const Vec2 &j) { vel += j * invMass; }

    void integrate(float dt, float gravityY = 980.0f, float floorY = 440.0f, float leftX = 20.0f, float rightX = 780.0f) {
        if (invMass == 0.0f) return;

        // Semi-Implicit Euler integration
        Vec2 accel = forceAccum * invMass + Vec2(0.0f, gravityY);
        vel += accel * dt;
        vel *= drag;
        pos += vel * dt;
        angleRad += angularVel * dt;
        forceAccum = Vec2(0, 0);

        // Floor bounce & rolling friction
        if (pos.y + radius > floorY) {
            pos.y = floorY - radius;
            if (vel.y > 0.0f) {
                vel.y = -vel.y * restitution;
                if (fabsf(vel.y) < 25.0f) vel.y = 0.0f;
            }
            vel.x *= friction;
            angularVel = (radius > 1.0f) ? (vel.x / radius) : 0.0f;
        }
        // Wall bounces
        if (pos.x - radius < leftX) {
            pos.x = leftX + radius;
            if (vel.x < 0.0f) vel.x = -vel.x * restitution;
        } else if (pos.x + radius > rightX) {
            pos.x = rightX - radius;
            if (vel.x > 0.0f) vel.x = -vel.x * restitution;
        }
    }

    static bool resolveCircleCollision(RigidBody2D &a, RigidBody2D &b) {
        Vec2 delta = b.pos - a.pos;
        float distSq = delta.lengthSq();
        float minDist = a.radius + b.radius;
        if (distSq >= minDist * minDist || distSq < 0.0001f) return false;

        float dist = sqrtf(distSq);
        Vec2 normal = delta / dist;
        float penetration = minDist - dist;

        float totalInvMass = a.invMass + b.invMass;
        if (totalInvMass <= 0.0f) return false;

        // Positional correction to prevent sinking
        Vec2 corr = normal * (penetration / totalInvMass * 0.85f);
        a.pos -= corr * a.invMass;
        b.pos += corr * b.invMass;

        // Impulse resolution
        Vec2 relVel = b.vel - a.vel;
        float velAlongNormal = relVel.dot(normal);
        if (velAlongNormal > 0.0f) return true;

        float e = min(a.restitution, b.restitution);
        float j = -(1.0f + e) * velAlongNormal / totalInvMass;
        Vec2 impulse = normal * j;
        a.vel -= impulse * a.invMass;
        b.vel += impulse * b.invMass;
        return true;
    }
};

struct Spring2D
{
    Vec2 anchor;
    Vec2 bob;
    Vec2 vel;
    float restLength;
    float stiffness; // k (Hooke's law)
    float damping;   // c (low damping = lively continuous harmonic bounce!)

    void init(float ax, float ay, float bx, float by, float restLen, float k = 155.0f, float damp = 0.28f) {
        anchor = Vec2(ax, ay);
        bob = Vec2(bx, by);
        vel = Vec2(120.0f, -180.0f); // Initial harmonic kick so it bounces immediately!
        restLength = restLen;
        stiffness = k;
        damping = damp;
    }

    void applyImpulse(const Vec2 &impulse) {
        vel += impulse;
    }

    // Two-way elastic collision between the spring bob and any RigidBody2D ball
    void collideWithCircle(RigidBody2D &ball, float bobRadius = 20.0f) {
        Vec2 delta = ball.pos - bob;
        float distSq = delta.lengthSq();
        float minDist = bobRadius + ball.radius;
        if (distSq < minDist * minDist && distSq > 0.001f) {
            float dist = sqrtf(distSq);
            Vec2 normal = delta / dist;
            float overlap = minDist - dist;
            bob      -= normal * (overlap * 0.5f);
            ball.pos += normal * (overlap * 0.5f);

            Vec2 relVel = ball.vel - vel;
            float sepVel = relVel.dot(normal);
            if (sepVel < 0.0f) {
                float j = -(1.0f + 0.85f) * sepVel * 0.5f;
                Vec2 imp = normal * j;
                vel      -= imp * 1.15f;
                ball.vel += imp;
            }
        }
    }

    void update(float dt, float gravityY = 680.0f) {
        Vec2 delta = bob - anchor;
        float dist = delta.length();
        if (dist > 0.001f) {
            float stretch = dist - restLength;
            Vec2 dir = delta / dist;
            Vec2 springForce = dir * (-stiffness * stretch) - vel * damping + Vec2(0.0f, gravityY);
            vel += springForce * dt;
            bob += vel * dt;

            // Keep spring bob inside screen bounds with elastic reflection
            if (bob.x < 40.0f)  { bob.x = 40.0f;  vel.x = fabsf(vel.x) * 0.8f; }
            if (bob.x > 760.0f) { bob.x = 760.0f; vel.x = -fabsf(vel.x) * 0.8f; }
            if (bob.y > 410.0f) { bob.y = 410.0f; vel.y = -fabsf(vel.y) * 0.85f; }
        }
    }

    void draw(RGBGameEngine &eng, int coils = 10, float width = 13.0f, uint16_t baseColor = Colors565::Silver) {
        Vec2 delta = bob - anchor;
        float len = delta.length();
        if (len < 2.0f) return;
        Vec2 dir = delta / len;
        Vec2 perp(-dir.y, dir.x);

        // Color shifts dynamically with spring tension (Green compressed -> Silver rest -> Orange/Red stretched)
        float strain = constrain((len - restLength) / (restLength * 0.6f), -1.0f, 1.0f);
        uint16_t coilCol = (strain > 0.15f)
            ? Color::lerp565(baseColor, Colors565::NeonOrange, (uint8_t)(strain * 220))
            : ((strain < -0.15f) ? Color::lerp565(baseColor, Colors565::NeonCyan, (uint8_t)(-strain * 220)) : baseColor);

        Vec2 prev = anchor;
        for (int i = 1; i <= coils; i++) {
            float t = (float)i / (coils + 1);
            float side = (i & 1) ? width : -width;
            Vec2 pt = anchor + dir * (len * t) + perp * side;
            eng.drawThickLine((int)prev.x, (int)prev.y, (int)pt.x, (int)pt.y, 2, coilCol);
            prev = pt;
        }
        eng.drawThickLine((int)prev.x, (int)prev.y, (int)bob.x, (int)bob.y, 2, coilCol);
        eng.fillCircle((int16_t)anchor.x, (int16_t)anchor.y, 6, Colors565::Gold);
        eng.drawCircle((int16_t)anchor.x, (int16_t)anchor.y, 6, Colors565::White);
    }
};

// ============================================================================
// 3. UNIVERSAL PORTABLE RADIAL FRACTURE & SHATTER WRAPPER ENGINE
//    - Wraps ANY object shape (Rectangle, Circle, Ellipse, Triangle, Polygon),
//      ANY Sprite/Image (const uint16_t*), or ANY live-rendered drawing lambda!
//    - Fractures radially from the exact hit point (hitX, hitY):
//      * Shards NEAR (hitX, hitY) are SMALL high-velocity impact splinters.
//      * Shards FARTHER from (hitX, hitY) grow RADIALLY LARGER (power-law r^1.82).
//    - Generates a true random mixture of UNEVEN TRAPEZIUMS (4-gon),
//      IRREGULAR QUADRILATERALS (4-gon), and UNEVEN TRIANGLES (3-gon).
//    - Average particle count (avgParticleCount) and blast intensity are
//      dynamically controllable at runtime!
// ============================================================================
enum class ShatterMaterial : uint8_t
{
    GLASS = 0,       // Tempered Window Pane: translucent glass trapeziums/quads/triangles with specular glint
    STONE,           // Heavy Stone Monolith / Boulder: chunky rock slabs & trapeziums with high mass
    WOOD,            // Wooden Crate / Plank: uneven fibrous splinters & trapeziums
    METAL_SPARKS,    // Molten / Heated Steel Plate: glowing Kelvin-cooling shrapnel polygons
    PIXEL_DISSOLVE   // Ceramic / Porcelain / Sprite Object: crisp polygon plates with sampled image colors
};

enum class ShatterShape : uint8_t
{
    RECTANGLE = 0,
    CIRCLE,
    ELLIPSE,
    TRIANGLE,
    POLYGON,
    SPRITE_IMAGE
};

struct ShatterTarget2D
{
    ShatterShape shape = ShatterShape::RECTANGLE;
    float x = 300.0f;       // Left X (Rect/Sprite) or Center X (Circle/Ellipse)
    float y = 150.0f;       // Top Y  (Rect/Sprite) or Center Y (Circle/Ellipse)
    float w = 160.0f;       // Width  (Rect/Sprite) or RadiusX  (Circle/Ellipse)
    float h = 160.0f;       // Height (Rect/Sprite) or RadiusY  (Circle/Ellipse)
    Vec2 polyVerts[8];      // Vertices for TRIANGLE (3) or POLYGON (3..8)
    uint8_t polyCount = 0;

    // Optional Sprite / Framebuffer Image sampling
    const uint16_t *pixels = nullptr;
    int16_t pixelW = 0;
    int16_t pixelH = 0;
    int16_t pixelStride = 0;      // Row stride in pixels (defaults to pixelW; 800 for live framebuffer capture!)
    uint16_t transparentKey = 0x0000;
    bool cullTransparent = false; // When true, shards landing on transparent/background pixels are culled
    uint16_t baseColor = Colors565::ElectricBlue;

    static ShatterTarget2D makeRect(float rx, float ry, float rw, float rh, uint16_t col = Colors565::ElectricBlue) {
        ShatterTarget2D t;
        t.shape = ShatterShape::RECTANGLE;
        t.x = rx; t.y = ry; t.w = rw; t.h = rh; t.baseColor = col;
        return t;
    }

    static ShatterTarget2D makeCircle(float cx, float cy, float radius, uint16_t col = Colors565::Silver) {
        ShatterTarget2D t;
        t.shape = ShatterShape::CIRCLE;
        t.x = cx; t.y = cy; t.w = radius; t.h = radius; t.baseColor = col;
        return t;
    }

    static ShatterTarget2D makeEllipse(float cx, float cy, float rx, float ry, uint16_t col = Colors565::DarkOrange) {
        ShatterTarget2D t;
        t.shape = ShatterShape::ELLIPSE;
        t.x = cx; t.y = cy; t.w = rx; t.h = ry; t.baseColor = col;
        return t;
    }

    static ShatterTarget2D makeTriangle(Vec2 a, Vec2 b, Vec2 c, uint16_t col = Colors565::Crimson) {
        ShatterTarget2D t;
        t.shape = ShatterShape::TRIANGLE;
        t.polyVerts[0] = a; t.polyVerts[1] = b; t.polyVerts[2] = c;
        t.polyCount = 3;
        float minX = min(a.x, min(b.x, c.x)), maxX = max(a.x, max(b.x, c.x));
        float minY = min(a.y, min(b.y, c.y)), maxY = max(a.y, max(b.y, c.y));
        t.x = minX; t.y = minY; t.w = max(8.0f, maxX - minX); t.h = max(8.0f, maxY - minY);
        t.baseColor = col;
        return t;
    }

    static ShatterTarget2D makePolygon(const Vec2 *pts, uint8_t count, uint16_t col = Colors565::Gold) {
        ShatterTarget2D t;
        t.shape = ShatterShape::POLYGON;
        t.polyCount = min((uint8_t)8, max((uint8_t)3, count));
        float minX = pts[0].x, maxX = pts[0].x, minY = pts[0].y, maxY = pts[0].y;
        for (uint8_t i = 0; i < t.polyCount; i++) {
            t.polyVerts[i] = pts[i];
            minX = min(minX, pts[i].x); maxX = max(maxX, pts[i].x);
            minY = min(minY, pts[i].y); maxY = max(maxY, pts[i].y);
        }
        t.x = minX; t.y = minY; t.w = max(8.0f, maxX - minX); t.h = max(8.0f, maxY - minY);
        t.baseColor = col;
        return t;
    }

    static ShatterTarget2D makeSprite(const uint16_t *img, int16_t sw, int16_t sh,
                                      float drawX, float drawY, float renderW = 0.0f, float renderH = 0.0f,
                                      uint16_t transKey = 0x0000, uint16_t fallbackCol = Colors565::White) {
        ShatterTarget2D t;
        t.shape = ShatterShape::SPRITE_IMAGE;
        t.x = drawX; t.y = drawY;
        t.w = (renderW > 1.0f) ? renderW : (float)sw;
        t.h = (renderH > 1.0f) ? renderH : (float)sh;
        t.pixels = img;
        t.pixelW = sw;
        t.pixelH = sh;
        t.pixelStride = sw;
        t.transparentKey = transKey;
        t.cullTransparent = true;
        t.baseColor = fallbackCol;
        return t;
    }

    // Wrap ANY region of the live RGB565 framebuffer (e.g. right after drawing a procedural creature/object)
    static ShatterTarget2D makeFromFramebuffer(const uint16_t *framebuf, int16_t fbWidth, int16_t fbHeight,
                                               float boxX, float boxY, float boxW, float boxH,
                                               ShatterShape boundaryShape = ShatterShape::RECTANGLE,
                                               uint16_t fallbackCol = Colors565::ElectricBlue) {
        ShatterTarget2D t;
        int16_t ix = (int16_t)constrain((int)boxX, 0, fbWidth - 2);
        int16_t iy = (int16_t)constrain((int)boxY, 0, fbHeight - 2);
        int16_t iw = (int16_t)constrain((int)boxW, 4, fbWidth - ix);
        int16_t ih = (int16_t)constrain((int)boxH, 4, fbHeight - iy);

        t.shape = boundaryShape;
        if (boundaryShape == ShatterShape::CIRCLE || boundaryShape == ShatterShape::ELLIPSE) {
            t.x = boxX + boxW * 0.5f;
            t.y = boxY + boxH * 0.5f;
            t.w = boxW * 0.5f;
            t.h = boxH * 0.5f;
        } else if (boundaryShape == ShatterShape::TRIANGLE) {
            t.polyVerts[0] = Vec2(boxX + boxW * 0.5f, boxY);
            t.polyVerts[1] = Vec2(boxX + boxW, boxY + boxH);
            t.polyVerts[2] = Vec2(boxX, boxY + boxH);
            t.polyCount = 3;
            t.x = boxX; t.y = boxY; t.w = boxW; t.h = boxH;
        } else {
            t.x = (float)ix; t.y = (float)iy; t.w = (float)iw; t.h = (float)ih;
        }

        if (framebuf) {
            t.pixels = &framebuf[iy * fbWidth + ix];
            t.pixelW = iw;
            t.pixelH = ih;
            t.pixelStride = fbWidth;
        }
        t.cullTransparent = false;
        t.baseColor = fallbackCol;
        return t;
    }
};

struct Shard2D
{
    Vec2 pos;
    Vec2 vel;
    Vec2 localVerts[4]; // Up to 4 vertices relative to centroid: 3 = uneven triangle, 4 = uneven trapezium / quadrilateral!
    uint8_t vertCount;  // 3 or 4
    float area;         // True polygon surface area (smaller near impact, larger radially outward!)
    float angle;
    float angularVel;
    float tumblePhase;  // Simulates 3D out-of-plane tumbling foreshortening
    float tumbleSpeed;
    float releaseDelay; // Crack propagation delay (impact center breaks first, outer slabs peel ms later)
    float boundingR;    // Distance to farthest vertex for accurate floor contact
    float heatKelvin;
    uint16_t baseColor;
    uint16_t edgeColor;
    uint8_t alpha;
    bool settled;
    bool active;
};

class ShatterSystem2D
{
public:
    static constexpr int MAX_SHARDS = 128;
    Shard2D shards[MAX_SHARDS];
    ShatterMaterial material;
    int defaultAvgParticles = 48; // Controllable average shard count per break!
    int lastSpawnedCount    = 0;
    float gravity;
    float bounce;
    float floorFriction;
    float floorY;
    bool active;

    ShatterSystem2D()
        : material(ShatterMaterial::GLASS), defaultAvgParticles(48), lastSpawnedCount(0),
          gravity(1150.0f), bounce(0.32f), floorFriction(0.82f), floorY(438.0f), active(false)
    {
        clear();
    }

    void clear() {
        active = false;
        lastSpawnedCount = 0;
        for (int i = 0; i < MAX_SHARDS; i++) shards[i].active = false;
    }

    void setAvgParticleCount(int avgCount) {
        defaultAvgParticles = constrain(avgCount, 8, MAX_SHARDS);
    }

    // Draw the intact physical object before impact (Window Pane, Stone Slab, Wood Crate, etc.)
    static void drawIntactObject(RGBGameEngine &eng, float centerX, float centerY, float halfSize, ShatterMaterial mat, uint16_t baseColor)
    {
        int16_t w = (int16_t)(halfSize * 2.1f);
        int16_t h = (int16_t)(halfSize * 1.95f);
        int16_t x = (int16_t)centerX - w / 2;
        int16_t y = (int16_t)centerY - h / 2;

        if (mat == ShatterMaterial::GLASS) {
            eng.fillRectAlpha(x, y, w, h, baseColor, 135);
            eng.drawThickLine(x + 18, y + h - 22, x + w - 28, y + 18, 6, Colors565::White, true);
            eng.drawThickLine(x + 42, y + h - 18, x + w - 16, y + 42, 3, Colors565::LightSkyBlue, true);
            eng.drawThickRect(x, y, w, h, 3, Colors565::White);
        } else if (mat == ShatterMaterial::STONE) {
            eng.fillRect(x, y, w, h, baseColor);
            eng.fillRect(x + 6, y + 6, w - 12, h - 12, Color::scaleBrightness565(baseColor, 215));
            eng.drawThickRect(x, y, w, h, 3, Colors565::LightGray);
        } else if (mat == ShatterMaterial::WOOD) {
            eng.fillRect(x, y, w, h, baseColor);
            uint16_t darkWood = Color::scaleBrightness565(baseColor, 150);
            for (int px = x + 28; px < x + w; px += 28) {
                eng.drawFastVLine(px, y, h, darkWood);
            }
            eng.drawThickRect(x, y, w, h, 4, darkWood);
            eng.drawThickLine(x + 6, y + 6, x + w - 6, y + h - 6, 5, darkWood, false);
        } else {
            eng.fillRect(x, y, w, h, baseColor);
            eng.drawThickRect(x, y, w, h, 3, Colors565::White);
        }
    }

private:
    // Compute exact ray distance from (hitX, hitY) along unit direction (dx, dy) to the wrapped object's outer boundary!
    static float computeRayBoundaryDist(const ShatterTarget2D &target, float hitX, float hitY, float dx, float dy)
    {
        switch (target.shape)
        {
            case ShatterShape::CIRCLE: {
                float ocx = hitX - target.x;
                float ocy = hitY - target.y;
                float r   = max(6.0f, target.w);
                float b   = ocx * dx + ocy * dy;
                float c   = ocx * ocx + ocy * ocy - r * r;
                float disc = max(0.0f, b * b - c);
                return max(6.0f, -b + sqrtf(disc));
            }

            case ShatterShape::ELLIPSE: {
                float rx = max(6.0f, target.w);
                float ry = max(6.0f, target.h);
                float ox = (hitX - target.x) / rx;
                float oy = (hitY - target.y) / ry;
                float vx = dx / rx;
                float vy = dy / ry;
                float a  = vx * vx + vy * vy;
                float b  = ox * vx + oy * vy;
                float c  = ox * ox + oy * oy - 1.0f;
                float disc = max(0.0f, b * b - a * c);
                return max(6.0f, (-b + sqrtf(disc)) / max(0.0001f, a));
            }

            case ShatterShape::TRIANGLE:
            case ShatterShape::POLYGON: {
                float minT = 1e6f;
                int n = (target.polyCount >= 3) ? target.polyCount : 3;
                for (int i = 0; i < n; i++) {
                    Vec2 A = target.polyVerts[i];
                    Vec2 B = target.polyVerts[(i + 1) % n];
                    Vec2 E = B - A;
                    // Solve hit + t * d = A + u * E
                    float denom = dx * E.y - dy * E.x;
                    if (fabsf(denom) > 0.0001f) {
                        float ax = A.x - hitX;
                        float ay = A.y - hitY;
                        float t = (ax * E.y - ay * E.x) / denom;
                        float u = (ax * dy  - ay * dx)  / denom;
                        if (t > 1.0f && u >= -0.02f && u <= 1.02f) {
                            minT = min(minT, t);
                        }
                    }
                }
                if (minT < 1e5f) return max(6.0f, minT);
                // Fallback to bounding box if hit point was slightly outside polygon
                break;
            }

            default:
                break;
        }

        // RECTANGLE or SPRITE_IMAGE bounding box ray intersection
        float left   = target.x;
        float right  = target.x + max(8.0f, target.w);
        float top    = target.y;
        float bottom = target.y + max(8.0f, target.h);

        float tx = (dx > 0.0001f) ? ((right - hitX) / dx) : ((dx < -0.0001f) ? ((left - hitX) / dx) : 1e6f);
        float ty = (dy > 0.0001f) ? ((bottom - hitY) / dy) : ((dy < -0.0001f) ? ((top - hitY) / dy) : 1e6f);
        return max(6.0f, min(tx, ty));
    }

    // Emit a single 3-gon (uneven triangle) or 4-gon (uneven trapezium / irregular quadrilateral) shard
    void addPolygonShard(
        const ShatterTarget2D &target,
        const Vec2 *verts,
        uint8_t vCount,
        float hitX, float hitY,
        float maxObjRadius,
        float intensity)
    {
        if (lastSpawnedCount >= MAX_SHARDS || vCount < 3) return;

        // Compute centroid and true polygon area via Shoelace formula
        Vec2 centroid(0.0f, 0.0f);
        float signedArea2 = 0.0f;
        for (uint8_t i = 0; i < vCount; i++) {
            centroid += verts[i];
            const Vec2 &p0 = verts[i];
            const Vec2 &p1 = verts[(i + 1) % vCount];
            signedArea2 += (p0.x * p1.y - p1.x * p0.y);
        }
        centroid = centroid * (1.0f / (float)vCount);
        float polyArea = max(4.0f, fabsf(signedArea2) * 0.5f);

        // Determine bounding box origin for sprite/framebuffer pixel sampling
        float boxLeft = (target.shape == ShatterShape::CIRCLE || target.shape == ShatterShape::ELLIPSE)
                        ? (target.x - target.w) : target.x;
        float boxTop  = (target.shape == ShatterShape::CIRCLE || target.shape == ShatterShape::ELLIPSE)
                        ? (target.y - target.h) : target.y;
        float boxW    = (target.shape == ShatterShape::CIRCLE || target.shape == ShatterShape::ELLIPSE)
                        ? (target.w * 2.0f) : max(1.0f, target.w);
        float boxH    = (target.shape == ShatterShape::CIRCLE || target.shape == ShatterShape::ELLIPSE)
                        ? (target.h * 2.0f) : max(1.0f, target.h);

        uint16_t sampledCol = target.baseColor;
        if (target.pixels && target.pixelW > 0 && target.pixelH > 0) {
            int sx = constrain((int)(((centroid.x - boxLeft) / boxW) * target.pixelW), 0, target.pixelW - 1);
            int sy = constrain((int)(((centroid.y - boxTop)  / boxH) * target.pixelH), 0, target.pixelH - 1);
            int stride = (target.pixelStride > 0) ? target.pixelStride : target.pixelW;
            uint16_t c = target.pixels[sy * stride + sx];

            // If centroid lands on transparent key, probe the shard's vertices before culling
            if (c == target.transparentKey) {
                for (uint8_t vi = 0; vi < vCount; vi++) {
                    int vx = constrain((int)(((verts[vi].x - boxLeft) / boxW) * target.pixelW), 0, target.pixelW - 1);
                    int vy = constrain((int)(((verts[vi].y - boxTop)  / boxH) * target.pixelH), 0, target.pixelH - 1);
                    uint16_t vc = target.pixels[vy * stride + vx];
                    if (vc != target.transparentKey) { c = vc; break; }
                }
            }
            if (c == target.transparentKey && target.cullTransparent) {
                return; // Outside the sprite's non-transparent silhouette!
            }
            if (c != target.transparentKey) {
                sampledCol = c;
            }
        } else {
            uint8_t shade = (uint8_t)random(168, 255);
            sampledCol = Color::scaleBrightness565(target.baseColor, shade);
        }

        Shard2D &sh = shards[lastSpawnedCount++];
        sh.active    = true;
        sh.settled   = false;
        sh.vertCount = vCount;
        sh.pos       = centroid;
        sh.area      = polyArea;

        float maxR = 2.0f;
        for (uint8_t i = 0; i < vCount; i++) {
            sh.localVerts[i] = verts[i] - centroid;
            float r = sh.localVerts[i].length();
            if (r > maxR) maxR = r;
        }
        sh.boundingR   = maxR * 0.65f;
        sh.angle       = 0.0f;
        sh.tumblePhase = (float)random(0, 314) * 0.01f;
        sh.alpha       = (material == ShatterMaterial::GLASS) ? 180 : 255;
        sh.heatKelvin  = 7800.0f;
        sh.baseColor   = sampledCol;
        sh.edgeColor   = (material == ShatterMaterial::GLASS)
                         ? Colors565::White
                         : Color::scaleBrightness565(sampledCol, 115);

        // --------------------------------------------------------------------
        // RADIAL IMPACT PHYSICS:
        // - Near (hitX, hitY): Small area -> light mass + high proximity impulse
        //   => explosive radial velocity & rapid spin!
        // - Farther from (hitX, hitY): Large area -> heavy mass + lower impulse
        //   => slower heavy tumble and radial crack propagation delay!
        // --------------------------------------------------------------------
        Vec2 fromHit = centroid - Vec2(hitX, hitY);
        float dist   = fromHit.length();
        Vec2 dir     = (dist > 0.5f) ? (fromHit / dist) : Vec2((float)random(-100, 101) * 0.01f, -0.8f).normalized();

        float normDist = constrain(dist / max(12.0f, maxObjRadius), 0.0f, 1.0f);
        // Nonlinear impact falloff: maximum force right at hit point!
        float impactProx = 1.0f / (1.0f + normDist * normDist * 4.5f);
        // Smaller pieces (near impact) have higher inverse-mass acceleration
        float invMassBoost = constrain(32.0f / sqrtf(polyArea), 0.55f, 2.15f);

        sh.releaseDelay = normDist * 0.048f; // Crack wave propagates radially outward in 0..48ms

        float radialSpeed = (65.0f + 240.0f * impactProx * invMassBoost) * intensity;
        float tangJitter  = (float)random(-55, 56) * intensity;
        Vec2 tang(-dir.y, dir.x);

        sh.vel = dir * radialSpeed + tang * tangJitter + Vec2(0.0f, -45.0f * impactProx * intensity);
        sh.angularVel  = ((dir.x >= 0.0f ? 1.0f : -1.0f) * (1.8f + 7.5f * impactProx * invMassBoost)
                         + (float)random(-25, 26) * 0.1f) * intensity;
        sh.tumbleSpeed = (2.4f + 5.2f * impactProx * invMassBoost) * intensity;
    }

public:
    // ------------------------------------------------------------------------
    // UNIVERSAL RADIAL FRACTURE ENGINE FOR ANY WRAPPED TARGET SHAPE OR IMAGE
    // ------------------------------------------------------------------------
    void shatterTarget(
        const ShatterTarget2D &target,
        float impactX, float impactY,
        int avgParticleCount = -1,
        float intensity = 1.0f,
        ShatterMaterial mat = ShatterMaterial::GLASS)
    {
        clear();
        material = mat;
        active = true;
        intensity = constrain(intensity, 0.2f, 2.8f);

        int desiredAvg = (avgParticleCount > 0) ? avgParticleCount : defaultAvgParticles;
        // Add +/- 15% natural random variance around avgParticleCount so every break is unique
        int targetTotal = constrain(desiredAvg + (int)random(-desiredAvg / 6, desiredAvg / 6 + 1), 8, MAX_SHARDS);

        switch (mat) {
            case ShatterMaterial::GLASS:
                gravity = 1180.0f; bounce = 0.26f; floorFriction = 0.88f; break;
            case ShatterMaterial::STONE:
                gravity = 1580.0f; bounce = 0.16f; floorFriction = 0.72f; break;
            case ShatterMaterial::WOOD:
                gravity = 1120.0f; bounce = 0.30f; floorFriction = 0.78f; break;
            case ShatterMaterial::METAL_SPARKS:
                gravity = 1280.0f; bounce = 0.42f; floorFriction = 0.84f; break;
            case ShatterMaterial::PIXEL_DISSOLVE:
                gravity = 1220.0f; bounce = 0.24f; floorFriction = 0.86f; break;
        }

        // Clamp impact point inside the wrapped shape so radial cracks emanate from the hit point
        float hitX = impactX;
        float hitY = impactY;
        if (target.shape == ShatterShape::CIRCLE || target.shape == ShatterShape::ELLIPSE) {
            float dx = (impactX - target.x) / max(6.0f, target.w);
            float dy = (impactY - target.y) / max(6.0f, target.h);
            float d  = sqrtf(dx * dx + dy * dy);
            if (d > 0.78f) {
                hitX = target.x + (dx / d) * target.w * 0.78f;
                hitY = target.y + (dy / d) * target.h * 0.78f;
            }
        } else {
            float marginX = min(14.0f, target.w * 0.22f);
            float marginY = min(14.0f, target.h * 0.22f);
            hitX = constrain(impactX, target.x + marginX, target.x + target.w - marginX);
            hitY = constrain(impactY, target.y + marginY, target.y + target.h - marginY);
        }

        // Determine radial sector rays (S) and concentric fracture rings (R) from targetTotal
        // About 25% of cells split into 2 pieces, so S * R * 1.25 ~= targetTotal
        int S = constrain((int)(sqrtf((float)targetTotal * 1.65f) + random(-1, 2)), 5, 16);
        int R = constrain((int)(((float)targetTotal / (float)S) * 0.85f + 0.5f), 2, 7);

        static constexpr int MAX_S = 16;
        static constexpr int MAX_R = 7;

        Vec2 radialNodes[MAX_S][MAX_R + 1];
        float rayAngles[MAX_S];
        float rayMaxR[MAX_S];
        float maxObjRadius = 16.0f;

        // 1. Generate uneven radial crack angles around (hitX, hitY)
        float angleStep = 6.2831853f / (float)S;
        float basePhase = (float)random(0, 628) * 0.01f;
        for (int s = 0; s < S; s++) {
            float jitter = ((float)random(-34, 35) * 0.01f) * angleStep;
            rayAngles[s] = basePhase + s * angleStep + jitter;
            rayMaxR[s]   = computeRayBoundaryDist(target, hitX, hitY, cosf(rayAngles[s]), sinf(rayAngles[s]));
            if (rayMaxR[s] > maxObjRadius) maxObjRadius = rayMaxR[s];
        }

        // 2. Place fracture nodes along each ray using a power-law r_k = R_max * (k / R)^1.82
        //    so rings near (hitX, hitY) are tightly packed (SMALL shards) and outer rings
        //    are wide apart (LARGE shards radially farther out!)
        for (int s = 0; s < S; s++) {
            // Epicenter node (k = 0) with slight micro-jitter around (hitX, hitY)
            radialNodes[s][0] = Vec2(hitX + (float)random(-2, 3), hitY + (float)random(-2, 3));

            for (int k = 1; k <= R; k++) {
                float u = powf((float)k / (float)R, 1.82f);
                // Add per-node radial & angular perturbation so cells form uneven trapeziums & irregular quadrilaterals!
                float rJitter = (k < R) ? (1.0f + ((float)random(-18, 19) * 0.01f) * (1.0f / (float)R)) : 1.0f;
                float aJitter = (k < R) ? (((float)random(-22, 23) * 0.01f) * angleStep) : 0.0f;

                float theta = rayAngles[s] + aJitter;
                float rMax  = (k == R) ? computeRayBoundaryDist(target, hitX, hitY, cosf(theta), sinf(theta))
                                       : (rayMaxR[s] * rJitter);
                float rDist = max(3.0f, rMax * u);

                radialNodes[s][k] = Vec2(hitX + cosf(theta) * rDist, hitY + sinf(theta) * rDist);
            }
        }

        // 3. Emit Shards from Radial Web:
        //    Mix of Uneven Triangles (3-gon), Uneven Trapeziums (4-gon), and Irregular Quadrilaterals (4-gon)!
        for (int k = 0; k < R && lastSpawnedCount < MAX_SHARDS; k++) {
            for (int s = 0; s < S && lastSpawnedCount < MAX_SHARDS; s++) {
                int sNext = (s + 1) % S;

                if (k == 0) {
                    // INNERMOST IMPACT ZONE (Right at hitX, hitY -> Smallest pieces!):
                    // 50% chance: uneven acute impact triangle (3-gon)
                    // 50% chance: truncated impact tip -> tiny core triangle (3-gon) + tiny uneven trapezium (4-gon)!
                    Vec2 centerPt = radialNodes[s][0];
                    Vec2 outerA   = radialNodes[s][1];
                    Vec2 outerB   = radialNodes[sNext][1];

                    if ((random(0, 100) < 52) && (lastSpawnedCount + 1 < MAX_SHARDS)) {
                        float tCutA = 0.35f + (float)random(0, 22) * 0.01f;
                        float tCutB = 0.35f + (float)random(0, 22) * 0.01f;
                        Vec2 midA = Vec2::lerp(centerPt, outerA, tCutA);
                        Vec2 midB = Vec2::lerp(centerPt, outerB, tCutB);

                        Vec2 triVerts[3]  = { centerPt, midA, midB };
                        Vec2 trapVerts[4] = { midA, outerA, outerB, midB };
                        addPolygonShard(target, triVerts,  3, hitX, hitY, maxObjRadius, intensity);
                        addPolygonShard(target, trapVerts, 4, hitX, hitY, maxObjRadius, intensity);
                    } else {
                        Vec2 triVerts[3] = { centerPt, outerA, outerB };
                        addPolygonShard(target, triVerts, 3, hitX, hitY, maxObjRadius, intensity);
                    }
                    continue;
                }

                // RADIAL RINGS k = 1 .. R-1 (Progressively LARGER pieces radially outward!)
                Vec2 p00 = radialNodes[s][k];
                Vec2 p10 = radialNodes[sNext][k];
                Vec2 p11 = radialNodes[sNext][k + 1];
                Vec2 p01 = radialNodes[s][k + 1];

                int fractureMode = random(0, 100);

                if (fractureMode < 42) {
                    // MODE 1 (42%): Intact Uneven Trapezium (4-gon: diverging radial sides + parallel-ish bases)
                    Vec2 trapVerts[4] = { p00, p10, p11, p01 };
                    addPolygonShard(target, trapVerts, 4, hitX, hitY, maxObjRadius, intensity);
                }
                else if (fractureMode < 74) {
                    // MODE 2 (32%): Irregular General Quadrilateral (4-gon: skewed non-parallel sides)
                    Vec2 skew00 = Vec2::lerp(p00, p01, (float)random(0, 24) * 0.01f);
                    Vec2 skew10 = Vec2::lerp(p10, p11, (float)random(-18, 25) * 0.01f);
                    Vec2 quadVerts[4] = { skew00, skew10, p11, p01 };
                    addPolygonShard(target, quadVerts, 4, hitX, hitY, maxObjRadius, intensity);
                }
                else if (fractureMode < 90 && (lastSpawnedCount + 1 < MAX_SHARDS)) {
                    // MODE 3 (16%): Oblique Edge-to-Corner Cleavage -> 1 Uneven Triangle (3-gon) + 1 Uneven Trapezium/Quad (4-gon)!
                    float splitT = 0.28f + (float)random(0, 44) * 0.01f;
                    Vec2 edgeMid = Vec2::lerp(p01, p11, splitT);

                    Vec2 triVerts[3]  = { p00, edgeMid, p01 };
                    Vec2 quadVerts[4] = { p00, p10, p11, edgeMid };
                    addPolygonShard(target, triVerts,  3, hitX, hitY, maxObjRadius, intensity);
                    addPolygonShard(target, quadVerts, 4, hitX, hitY, maxObjRadius, intensity);
                }
                else {
                    // MODE 4 (10%): Diagonal Cleavage -> 2 Uneven Triangles (3-gon + 3-gon)
                    if (random(0, 2) == 0) {
                        Vec2 triA[3] = { p00, p10, p11 };
                        Vec2 triB[3] = { p00, p11, p01 };
                        addPolygonShard(target, triA, 3, hitX, hitY, maxObjRadius, intensity);
                        addPolygonShard(target, triB, 3, hitX, hitY, maxObjRadius, intensity);
                    } else {
                        Vec2 triA[3] = { p00, p10, p01 };
                        Vec2 triB[3] = { p10, p11, p01 };
                        addPolygonShard(target, triA, 3, hitX, hitY, maxObjRadius, intensity);
                        addPolygonShard(target, triB, 3, hitX, hitY, maxObjRadius, intensity);
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // Universal Wrapper: Draw ANY object via a lambda/callable, capture its
    // live rendered pixels & shape from the framebuffer, and shatter it!
    // ------------------------------------------------------------------------
    template <typename DrawCallable>
    void shatterWrappedDraw(
        RGBGameEngine &eng,
        float boxX, float boxY, float boxW, float boxH,
        float hitX, float hitY,
        DrawCallable drawFn,
        int avgParticleCount = 48,
        float intensity = 1.25f,
        ShatterMaterial mat = ShatterMaterial::GLASS,
        ShatterShape boundaryShape = ShatterShape::RECTANGLE,
        uint16_t fallbackColor = Colors565::ElectricBlue)
    {
        // Render the object onto the current draw buffer so its exact procedural colors are present
        drawFn();
        ShatterTarget2D target = ShatterTarget2D::makeFromFramebuffer(
            eng.getDrawBuffer(), 800, 480, boxX, boxY, boxW, boxH, boundaryShape, fallbackColor
        );
        shatterTarget(target, hitX, hitY, avgParticleCount, intensity, mat);
    }

    // Backward-compatible overload (automatically uses radial uneven trapezium/quad/triangle fracture!)
    void triggerShatter(
        float centerX, float centerY, float halfSize,
        float impactX, float impactY,
        ShatterMaterial mat = ShatterMaterial::GLASS,
        float intensity = 1.0f,
        uint16_t baseColor = Colors565::ElectricBlue,
        const uint16_t *spritePixels = nullptr, int16_t spriteW = 0, int16_t spriteH = 0,
        int avgParticleCount = -1,
        ShatterShape shape = ShatterShape::RECTANGLE)
    {
        float objW = (spriteW > 0) ? (float)spriteW : (halfSize * 2.1f);
        float objH = (spriteH > 0) ? (float)spriteH : (halfSize * 1.95f);

        ShatterTarget2D target;
        if (spritePixels && spriteW > 0 && spriteH > 0) {
            target = ShatterTarget2D::makeSprite(spritePixels, spriteW, spriteH,
                                                 centerX - objW * 0.5f, centerY - objH * 0.5f,
                                                 objW, objH, 0x0000, baseColor);
        } else if (shape == ShatterShape::CIRCLE) {
            target = ShatterTarget2D::makeCircle(centerX, centerY, halfSize, baseColor);
        } else if (shape == ShatterShape::ELLIPSE) {
            target = ShatterTarget2D::makeEllipse(centerX, centerY, objW * 0.5f, objH * 0.5f, baseColor);
        } else if (shape == ShatterShape::TRIANGLE) {
            target = ShatterTarget2D::makeTriangle(
                Vec2(centerX, centerY - objH * 0.5f),
                Vec2(centerX + objW * 0.5f, centerY + objH * 0.5f),
                Vec2(centerX - objW * 0.5f, centerY + objH * 0.5f),
                baseColor);
        } else {
            target = ShatterTarget2D::makeRect(centerX - objW * 0.5f, centerY - objH * 0.5f, objW, objH, baseColor);
        }

        int pCount = (avgParticleCount > 0) ? avgParticleCount : (int)(defaultAvgParticles * constrain(intensity, 0.6f, 1.8f));
        shatterTarget(target, impactX, impactY, pCount, intensity, mat);
    }

    void update(float dt) {
        if (!active) return;

        for (int i = 0; i < MAX_SHARDS; i++) {
            Shard2D &sh = shards[i];
            if (!sh.active || sh.settled) continue;

            if (sh.releaseDelay > 0.0f) {
                sh.releaseDelay -= dt;
                continue;
            }

            sh.vel.y += gravity * dt;
            sh.vel.x *= 0.996f;
            sh.pos += sh.vel * dt;
            sh.angle += sh.angularVel * dt;
            sh.tumblePhase += sh.tumbleSpeed * dt;

            if (material == ShatterMaterial::METAL_SPARKS) {
                sh.heatKelvin = max(1200.0f, sh.heatKelvin - 2400.0f * dt);
                sh.baseColor = Color::kelvin565((uint16_t)sh.heatKelvin);
            }

            float bottomLimit = floorY - sh.boundingR * 0.45f;
            if (sh.pos.y >= bottomLimit) {
                sh.pos.y = bottomLimit;
                if (sh.vel.y > 0.0f) {
                    sh.vel.y = -sh.vel.y * bounce;
                }
                sh.vel.x *= floorFriction;
                sh.angularVel *= floorFriction;
                sh.tumbleSpeed *= 0.75f;

                if (fabsf(sh.vel.y) < 28.0f && fabsf(sh.vel.x) < 12.0f) {
                    sh.vel = Vec2(0.0f, 0.0f);
                    sh.angularVel = 0.0f;
                    sh.tumbleSpeed = 0.0f;
                    sh.settled = true;
                }
            }

            if (sh.pos.x < 18.0f)  { sh.pos.x = 18.0f;  sh.vel.x = fabsf(sh.vel.x) * 0.5f; }
            if (sh.pos.x > 782.0f) { sh.pos.x = 782.0f; sh.vel.x = -fabsf(sh.vel.x) * 0.5f; }
        }
    }

    void draw(RGBGameEngine &eng) {
        if (!active) return;

        for (int i = 0; i < MAX_SHARDS; i++) {
            const Shard2D &sh = shards[i];
            if (!sh.active) continue;

            float foreshorten = sh.settled ? 0.75f : (0.35f + 0.65f * fabsf(cosf(sh.tumblePhase)));
            Vec2 v[4];
            for (uint8_t k = 0; k < sh.vertCount; k++) {
                Vec2 lv(sh.localVerts[k].x, sh.localVerts[k].y * foreshorten);
                v[k] = sh.pos + lv.rotated(sh.angle);
            }

            int16_t x0 = (int16_t)v[0].x, y0 = (int16_t)v[0].y;
            int16_t x1 = (int16_t)v[1].x, y1 = (int16_t)v[1].y;
            int16_t x2 = (int16_t)v[2].x, y2 = (int16_t)v[2].y;

            if (sh.vertCount == 4) {
                int16_t x3 = (int16_t)v[3].x, y3 = (int16_t)v[3].y;
                if (material == ShatterMaterial::GLASS) {
                    bool glint = (!sh.settled) && (foreshorten > 0.92f);
                    uint16_t col = glint ? Colors565::White : sh.baseColor;
                    eng.fillTriangleAlpha(x0, y0, x1, y1, x2, y2, col, sh.alpha);
                    eng.fillTriangleAlpha(x0, y0, x2, y2, x3, y3, col, sh.alpha);
                } else {
                    uint8_t tiltShade = (uint8_t)(155 + 100 * foreshorten);
                    uint16_t litCol = Color::scaleBrightness565(sh.baseColor, tiltShade);
                    eng.fillQuad(x0, y0, x1, y1, x2, y2, x3, y3, litCol);
                }
                // Draw only the 4 outer perimeter edges of the uneven trapezium / quadrilateral!
                eng.drawLine(x0, y0, x1, y1, sh.edgeColor);
                eng.drawLine(x1, y1, x2, y2, sh.edgeColor);
                eng.drawLine(x2, y2, x3, y3, sh.edgeColor);
                eng.drawLine(x3, y3, x0, y0, sh.edgeColor);
            } else {
                if (material == ShatterMaterial::GLASS) {
                    bool glint = (!sh.settled) && (foreshorten > 0.92f);
                    uint16_t col = glint ? Colors565::White : sh.baseColor;
                    eng.fillTriangleAlpha(x0, y0, x1, y1, x2, y2, col, sh.alpha);
                    eng.drawTriangle(x0, y0, x1, y1, x2, y2, sh.edgeColor);
                } else {
                    uint8_t tiltShade = (uint8_t)(155 + 100 * foreshorten);
                    uint16_t litCol = Color::scaleBrightness565(sh.baseColor, tiltShade);
                    eng.fillTriangle(x0, y0, x1, y1, x2, y2, litCol);
                    eng.drawTriangle(x0, y0, x1, y1, x2, y2, sh.edgeColor);
                }
            }
        }
    }
};

// ============================================================================
// 4. PRESS & COMPRESSION ENGINE (VOLUME-PRESERVING PLUSH CUSHION & BOUNCE)
//    Uses 33 coupled surface spring nodes, volume-conserving shoulder lift &
//    side bulge, two-way elastic ball bounce, and ultra-fast vertical scanline
//    rendering (zero fillTriangle/fillQuad overhead!).
// ============================================================================
class SoftBodyCushion2D
{
public:
    static constexpr int TOP_NODES = 33;
    float surfaceY[TOP_NODES];
    float restY[TOP_NODES];
    float velY[TOP_NODES];
    bool  contactMask[TOP_NODES];

    float baseX, baseY, width, height;
    float sideBulge;      // Volume-preserving horizontal bulge when pressed down!
    float stiffness;      // Hookean restoring spring constant
    float waveSpread;     // Elastic neighbor coupling
    float damping;
    uint16_t bodyColor;
    uint16_t rimColor;

    void init(float x, float y, float w, float h, uint16_t col = Colors565::Crimson, uint16_t borderCol = Colors565::Gold) {
        baseX = x; baseY = y; width = w; height = h;
        sideBulge = 0.0f;
        stiffness = 185.0f;
        waveSpread = 420.0f;
        damping = 6.2f;
        bodyColor = col;
        rimColor = borderCol;

        for (int i = 0; i < TOP_NODES; i++) {
            float t = (float)i / (TOP_NODES - 1);
            // Plush crowned pillow arch at rest
            float arch = sinf(t * 3.14159265f) * 18.0f;
            restY[i] = y - arch;
            surfaceY[i] = restY[i];
            velY[i] = 0.0f;
            contactMask[i] = false;
        }
    }

    // Direct press/slam impulse at pressX
    void applyPress(float pressX, float pressRadius, float pressDepth) {
        float stepX = width / (TOP_NODES - 1);
        for (int i = 1; i < TOP_NODES - 1; i++) {
            float nodeX = baseX + i * stepX;
            float dx = fabsf(nodeX - pressX);
            if (dx < pressRadius) {
                float factor = 0.5f * (1.0f + cosf(3.14159265f * dx / pressRadius));
                velY[i] += pressDepth * factor * 9.5f;
            }
        }
    }

    // Two-way physical interaction between a RigidBody2D ball (or manual cursor ball) and the cushion!
    // Returns total upward elastic force exerted by the cushion on the ball.
    float interactBall(float ballX, float &ballY, float &ballVelY, float ballRadius, float dt, bool isManualHold, float intensity = 1.0f) {
        float stepX = width / (TOP_NODES - 1);
        float maxSinkY = baseY + height - 22.0f;
        float upwardForceSum = 0.0f;
        int contactNodes = 0;

        for (int i = 0; i < TOP_NODES; i++) contactMask[i] = false;

        for (int i = 1; i < TOP_NODES - 1; i++) {
            float nodeX = baseX + i * stepX;
            float dx = nodeX - ballX;
            // Broaden contact cradle slightly so the pillow wraps smoothly around the ball
            float cradleR = ballRadius * 1.18f;
            if (fabsf(dx) < cradleR) {
                float norm = dx / cradleR;
                float circleBottomY = ballY + ballRadius * sqrtf(max(0.0f, 1.0f - norm * norm));

                if (circleBottomY > surfaceY[i] - 2.0f) {
                    contactMask[i] = true;
                    contactNodes++;
                    float targetY = min(maxSinkY, circleBottomY);
                    float penetration = targetY - surfaceY[i];

                    if (penetration > 0.0f) {
                        // Push cushion surface down smoothly with the ball
                        surfaceY[i] += penetration * 0.78f;
                        if (ballVelY > velY[i]) {
                            velY[i] = ballVelY * 0.65f;
                        }
                    }

                    // Compute upward Hookean spring reaction from how far this node is compressed below restY
                    float compression = surfaceY[i] - restY[i];
                    if (compression > 0.0f) {
                        upwardForceSum += compression * stiffness;
                    }
                }
            }
        }

        // If the ball is in free-fall/bounce mode (not held stationary by WASD), apply cushion's upward elastic bounce!
        if (!isManualHold && contactNodes > 0 && upwardForceSum > 0.0f) {
            float avgSpringAccel = (upwardForceSum / max(1, contactNodes)) * (2.6f * constrain(intensity, 0.5f, 2.2f));
            ballVelY -= avgSpringAccel * dt;
            // Cushion internal damping absorbs a small fraction of impact velocity on the way down
            if (ballVelY > 0.0f) {
                ballVelY *= (1.0f - min(0.35f, 3.2f * dt));
            }
        }
        return upwardForceSum;
    }

    void update(float dt) {
        if (dt > 0.03f) dt = 0.03f;

        // 2 substeps for rock-solid, butter-smooth elastic wave propagation
        float subDt = dt * 0.5f;
        for (int step = 0; step < 2; step++) {
            float displacedArea = 0.0f;
            int unpressedCount = 0;

            for (int i = 1; i < TOP_NODES - 1; i++) {
                float indent = surfaceY[i] - restY[i];
                if (indent > 0.0f) displacedArea += indent;
                if (!contactMask[i]) unpressedCount++;
            }

            // Volume conservation: pressing down in the middle puffs unpressed pillow shoulders upward!
            float shoulderLift = (unpressedCount > 0) ? ((displacedArea / TOP_NODES) * 0.48f) : 0.0f;

            for (int i = 1; i < TOP_NODES - 1; i++) {
                float targetRest = restY[i] - (contactMask[i] ? 0.0f : shoulderLift);
                float disp = surfaceY[i] - targetRest;
                float neighborLaplacian = (surfaceY[i - 1] + surfaceY[i + 1]) - 2.0f * surfaceY[i];

                float accel = (-stiffness * disp) + (waveSpread * neighborLaplacian) - (damping * velY[i]);
                velY[i] += accel * subDt;
            }

            float maxSinkY = baseY + height - 20.0f;
            float totalIndent = 0.0f;
            for (int i = 1; i < TOP_NODES - 1; i++) {
                surfaceY[i] += velY[i] * subDt;
                if (surfaceY[i] > maxSinkY) {
                    surfaceY[i] = maxSinkY;
                    velY[i] *= -0.25f;
                }
                float indent = surfaceY[i] - restY[i];
                if (indent > 0.0f) totalIndent += indent;
            }

            // Smoothly filter side bulge for volume-conserving lateral expansion
            float targetBulge = (totalIndent / TOP_NODES) * 0.95f;
            sideBulge = sideBulge * 0.8f + targetBulge * 0.2f;
        }
    }

    // Ultra-fast single-pass scanline renderer (zero fillTriangle overhead!)
    void draw(RGBGameEngine &eng) {
        int16_t bottomY  = (int16_t)(baseY + height);
        int16_t lx       = (int16_t)baseX;
        int16_t rx       = (int16_t)(baseX + width);
        int16_t bulgeMax = (int16_t)(14.0f + sideBulge);

        uint16_t highlightCol = Color::blend565(Colors565::White, bodyColor, 95);
        uint16_t midCol       = bodyColor;
        uint16_t shadowCol    = Color::scaleBrightness565(bodyColor, 145);
        uint16_t deepBaseCol  = Color::scaleBrightness565(bodyColor, 95);

        // 1. Draw volume-preserving curved plush left & right side bolsters using fast horizontal scanlines
        int16_t topLeftY  = (int16_t)surfaceY[0];
        int16_t topRightY = (int16_t)surfaceY[TOP_NODES - 1];
        int16_t sideH     = max((int16_t)1, (int16_t)(bottomY - topLeftY));

        for (int16_t y = topLeftY; y <= bottomY; y++) {
            float frac = (float)(y - topLeftY) / (float)sideH;
            int16_t bOff = (int16_t)(sinf(frac * 3.14159265f) * bulgeMax);
            uint16_t rowCol = (frac < 0.18f) ? highlightCol : ((frac < 0.68f) ? midCol : ((frac < 0.88f) ? shadowCol : deepBaseCol));
            if (bOff > 0) {
                eng.drawFastHLine(lx - bOff, y, bOff + 1, rowCol);
                eng.drawFastHLine(rx,        y, bOff + 1, rowCol);
            }
            // Stitched gold side piping pixels
            eng.drawPixel(lx - bOff, y, rimColor);
            eng.drawPixel(lx - bOff + 1, y, rimColor);
            eng.drawPixel(rx + bOff, y, rimColor);
            eng.drawPixel(rx + bOff - 1, y, rimColor);
        }

        // 2. Draw main cushion body using fast vertical columns (interpolating surfaceY smoothly)
        float invW = (float)(TOP_NODES - 1) / width;
        for (int16_t x = lx; x <= rx; x++) {
            float fIdx = (x - lx) * invW;
            int idx = constrain((int)fIdx, 0, TOP_NODES - 2);
            float fracX = fIdx - idx;
            int16_t topY = (int16_t)(surfaceY[idx] + (surfaceY[idx + 1] - surfaceY[idx]) * fracX);
            int16_t colH = bottomY - topY;
            if (colH <= 4) continue;

            int16_t hBand1 = max((int16_t)4, (int16_t)(colH * 0.16f));
            int16_t hBand2 = max((int16_t)4, (int16_t)(colH * 0.52f));
            int16_t hBand3 = max((int16_t)4, (int16_t)(colH * 0.20f));
            int16_t hBand4 = max((int16_t)1, (int16_t)(colH - hBand1 - hBand2 - hBand3));

            int16_t curY = topY;
            eng.drawFastVLine(x, curY, hBand1, highlightCol); curY += hBand1;
            eng.drawFastVLine(x, curY, hBand2, midCol);       curY += hBand2;
            eng.drawFastVLine(x, curY, hBand3, shadowCol);    curY += hBand3;
            eng.drawFastVLine(x, curY, hBand4, deepBaseCol);

            // Top golden stitched piping seam (3px thick)
            eng.drawFastVLine(x, topY - 1, 3, rimColor);
        }

        // 3. Draw plush recessed diamond tuft buttons that sink & move with the local surface!
        for (int i = 4; i < TOP_NODES - 4; i += 6) {
            int16_t tx = (int16_t)(baseX + (i * width) / (TOP_NODES - 1));
            int16_t ty = (int16_t)(surfaceY[i] + (bottomY - surfaceY[i]) * 0.42f);
            // Crease lines radiating from tuft button
            eng.drawLine(tx - 10, ty - 6, tx + 10, ty + 6, deepBaseCol);
            eng.drawLine(tx - 10, ty + 6, tx + 10, ty - 6, deepBaseCol);
            eng.fillCircle(tx, ty, 5, rimColor);
            eng.drawCircle(tx, ty, 5, deepBaseCol);
            eng.drawPixel(tx - 1, ty - 1, Colors565::White);
        }

        // Bottom wooden/gold base trim
        eng.drawFastHLine(lx, bottomY, (int16_t)width, rimColor);
        eng.drawFastHLine(lx, bottomY + 1, (int16_t)width, rimColor);
    }
};

// ============================================================================
// 5. HIGH-RESOLUTION WOVEN SILK / VELVET CLOTH & FLOWING NINJA SCARF
//    (29x19 Micro-Grid + Structural/Shear/Bending PBD Solver + 3D Fold Shading)
// ============================================================================
class VerletCloth2D
{
public:
    static constexpr int COLS = 29;
    static constexpr int ROWS = 19;

    Vec2 pos[ROWS][COLS];
    Vec2 oldPos[ROWS][COLS];
    bool pinned[ROWS][COLS];
    bool linkRight[ROWS][COLS];
    bool linkDown[ROWS][COLS];

    float spacing;
    float tearThreshold;
    uint16_t clothColor1;
    uint16_t clothColor2;

    // Remember last sphere position for 3D spherical bulge lighting on the fabric surface
    float lastColX = -1000.0f;
    float lastColY = -1000.0f;
    float lastColR = 0.0f;
    float colX = -1000.0f;
    float colY = -1000.0f;
    float colR = 0.0f;
    bool  colActive = false;

    void init(float startX, float startY, float cellSpacing = 12.0f, uint16_t col1 = Colors565::Crimson, uint16_t col2 = Colors565::Gold) {
        // Automatically convert legacy coarse spacing (e.g. 22px for 16 cols) to fine 29-col micro-spacing (~12px)
        if (cellSpacing > 15.0f) {
            spacing = (cellSpacing * 15.5f) / (float)(COLS - 1);
        } else {
            spacing = cellSpacing;
        }
        tearThreshold = spacing * 6.5f;
        clothColor1 = col1;
        clothColor2 = col2;
        colActive = false;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                pos[r][c] = Vec2(startX + c * spacing, startY + r * spacing);
                oldPos[r][c] = pos[r][c];
                // Pin top curtain rings every 2 columns + corners so top edge is securely hung
                pinned[r][c] = (r == 0 && (c % 2 == 0 || c == COLS - 1));
                linkRight[r][c] = (c < COLS - 1);
                linkDown[r][c]  = (r < ROWS - 1);
            }
        }
    }

    // Fast Padé / Jakobsen distance constraint relaxation (zero sqrtf overhead!)
    inline void relaxPair(int r1, int c1, int r2, int c2, float restLenSq, float stiffness) {
        float dx = pos[r2][c2].x - pos[r1][c1].x;
        float dy = pos[r2][c2].y - pos[r1][c1].y;
        float d2 = dx * dx + dy * dy;
        float diff = ((d2 - restLenSq) / (d2 + restLenSq + 0.001f)) * stiffness;
        float cx = dx * diff;
        float cy = dy * diff;

        bool p1 = pinned[r1][c1];
        bool p2 = pinned[r2][c2];
        if (!p1 && !p2) {
            pos[r1][c1].x += cx;
            pos[r1][c1].y += cy;
            pos[r2][c2].x -= cx;
            pos[r2][c2].y -= cy;
        } else if (!p1) {
            pos[r1][c1].x += cx * 2.0f;
            pos[r1][c1].y += cy * 2.0f;
        } else if (!p2) {
            pos[r2][c2].x -= cx * 2.0f;
            pos[r2][c2].y -= cy * 2.0f;
        }
    }

    void interactCircle(float cx, float cy, float radius, bool tearOnContact = false) {
        if (tearOnContact) {
            colActive = false;
            // Clean surgical blade cut radius (only cuts threads directly touched by the blade core)
            float cutR2 = (radius * 0.48f) * (radius * 0.48f);
            Vec2 center(cx, cy);
            for (int r = 1; r < ROWS; r++) {
                for (int c = 0; c < COLS; c++) {
                    float d2 = (pos[r][c] - center).lengthSq();
                    if (d2 < cutR2) {
                        linkRight[r][c] = false;
                        linkDown[r][c]  = false;
                    }
                }
            }
        } else {
            // Store active sphere so collision is solved simultaneously with cloth tension in update()!
            colX = cx;
            colY = cy;
            colR = radius;
            lastColX = cx;
            lastColY = cy;
            lastColR = radius;
            colActive = true;
        }
    }

    void update(float dt, float windX = 140.0f, float gravityY = 640.0f) {
        (void)dt;
        const float fixedDt2 = 0.016f * 0.016f;
        float t = millis() * 0.0036f;

        // 1. Verlet Integration + Coherent Aerodynamic Drapery Waves
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (pinned[r][c]) continue;
                Vec2 vel = (pos[r][c] - oldPos[r][c]) * 0.968f;
                // Clamp max per-step velocity so rapid cursor flicks never destabilize the weave
                float vSq = vel.lengthSq();
                if (vSq > 196.0f) {
                    vel = vel * (14.0f / sqrtf(vSq));
                }
                oldPos[r][c] = pos[r][c];

                // Natural traveling silk pleat wave across columns and rows
                float phase = t * 2.1f - c * 0.28f - r * 0.20f;
                float gustX = windX * (0.35f + 0.65f * sinf(phase));
                float liftY = -fabsf(windX) * 0.14f * cosf(phase);

                pos[r][c].x += vel.x + gustX * fixedDt2;
                pos[r][c].y += vel.y + (gravityY + liftY) * fixedDt2;
            }
        }

        const float structSq = spacing * spacing;
        const float shearSq  = structSq * 2.0f;
        const float bendSq   = structSq * 4.0f;
        const float r2Col    = colR * colR;

        // 2. Multi-Pass Position-Based Dynamics (Structural + Diagonal Shear + Bending + Sphere Collision)
        for (int iter = 0; iter < 7; iter++) {
            // 2a. Structural Horizontal (Weft) & Vertical (Warp) Constraints
            for (int r = 0; r < ROWS; r++) {
                for (int c = 0; c < COLS; c++) {
                    if (linkRight[r][c] && c + 1 < COLS) {
                        relaxPair(r, c, r, c + 1, structSq, 0.48f);
                    }
                    if (linkDown[r][c] && r + 1 < ROWS) {
                        relaxPair(r, c, r + 1, c, structSq, 0.48f);
                    }
                }
            }

            // 2b. Diagonal Shear & 2-Hop Bending Resistance (Prevents "creamy" scissoring/melting!)
            if ((iter & 1) == 0) {
                for (int r = 0; r < ROWS - 1; r++) {
                    for (int c = 0; c < COLS - 1; c++) {
                        if (linkRight[r][c] && linkDown[r][c] && linkRight[r + 1][c] && linkDown[r][c + 1]) {
                            relaxPair(r, c, r + 1, c + 1, shearSq, 0.32f);
                            relaxPair(r + 1, c, r, c + 1, shearSq, 0.32f);
                        }
                        if (c + 2 < COLS && linkRight[r][c] && linkRight[r][c + 1]) {
                            relaxPair(r, c, r, c + 2, bendSq, 0.22f);
                        }
                        if (r + 2 < ROWS && linkDown[r][c] && linkDown[r + 1][c]) {
                            relaxPair(r, c, r + 2, c, bendSq, 0.22f);
                        }
                    }
                }
            }

            // 2c. In-Solver Sphere Collision Projection (Fabric wraps tautly around the sphere!)
            if (colActive) {
                for (int r = 1; r < ROWS; r++) {
                    for (int c = 0; c < COLS; c++) {
                        float dx = pos[r][c].x - colX;
                        float dy = pos[r][c].y - colY;
                        float d2 = dx * dx + dy * dy;
                        if (d2 < r2Col && d2 > 0.0001f) {
                            float invLen = 1.0f / sqrtf(d2);
                            float targetX = colX + dx * invLen * colR;
                            float targetY = colY + dy * invLen * colR;
                            // Smoothly project node onto sphere surface and damp inward velocity
                            pos[r][c].x += (targetX - pos[r][c].x) * 0.78f;
                            pos[r][c].y += (targetY - pos[r][c].y) * 0.78f;
                            oldPos[r][c].x = pos[r][c].x - (targetX - pos[r][c].x) * 0.15f;
                            oldPos[r][c].y = pos[r][c].y - (targetY - pos[r][c].y) * 0.15f;
                        }
                    }
                }
            }
        }
        colActive = false;
    }

    void draw(RGBGameEngine &eng, bool filledShaded = true) {
        // Draw ornate golden curtain rod at the top
        int16_t rodLeft  = (int16_t)pos[0][0].x - 14;
        int16_t rodWidth = (int16_t)((COLS - 1) * spacing + 28);
        eng.fillRect(rodLeft, (int16_t)pos[0][0].y - 5, rodWidth, 6, Colors565::Gold);
        eng.drawFastHLine(rodLeft, (int16_t)pos[0][0].y - 5, rodWidth, Colors565::White);

        for (int r = 0; r < ROWS - 1; r++) {
            for (int c = 0; c < COLS - 1; c++) {
                bool triTopRight   = linkRight[r][c] && linkDown[r][c + 1];
                bool triBottomLeft = linkDown[r][c]  && linkRight[r + 1][c];

                if (filledShaded && (triTopRight || triBottomLeft)) {
                    // Royal gold brocade border along bottom hem and side edges
                    bool isHemBorder = (r == ROWS - 2 || c == 0 || c == COLS - 2);
                    uint16_t base = isHemBorder ? Colors565::Gold : clothColor1;

                    // Compute smooth 3D drapery normal lighting from micro-cell slope & pleat curvature
                    float dx = pos[r][c + 1].x - pos[r][c].x;
                    float dy = pos[r][c + 1].y - pos[r][c].y;
                    float vy = pos[r + 1][c].y - pos[r][c].y;
                    float vx = pos[r + 1][c].x - pos[r][c].x;

                    // Cylindrical vertical pleat wave + local shear/fold angle
                    float pleatWave = sinf(c * 0.65f + vx * 0.18f) * 28.0f;
                    int lightVal = (int)(172.0f + pleatWave + dy * 8.5f + (dx - spacing) * 9.0f + (spacing - vy) * 7.5f - vx * 3.5f);

                    // Add 3D spherical contour highlight where the collider sphere presses into the fabric
                    if (lastColR > 1.0f) {
                        float bdx = pos[r][c].x - lastColX;
                        float bdy = pos[r][c].y - lastColY;
                        float bd2 = bdx * bdx + bdy * bdy;
                        float bulgeR2 = (lastColR * 1.45f) * (lastColR * 1.45f);
                        if (bd2 < bulgeR2) {
                            float rim = 1.0f - (bd2 / bulgeR2);
                            lightVal += (int)(rim * 75.0f - (bdy / lastColR) * 28.0f);
                        }
                    }

                    uint16_t shadedCol;
                    if (lightVal > 255) {
                        uint8_t sheen = (uint8_t)min(175, (lightVal - 255) * 2);
                        shadedCol = Color::blend565(Colors565::White, base, sheen);
                    } else {
                        uint8_t br = (uint8_t)constrain(lightVal, 55, 255);
                        shadedCol = Color::scaleBrightness565(base, br);
                    }

                    int16_t x00 = (int16_t)pos[r][c].x,         y00 = (int16_t)pos[r][c].y;
                    int16_t x10 = (int16_t)pos[r][c + 1].x,     y10 = (int16_t)pos[r][c + 1].y;
                    int16_t x11 = (int16_t)pos[r + 1][c + 1].x, y11 = (int16_t)pos[r + 1][c + 1].y;
                    int16_t x01 = (int16_t)pos[r + 1][c].x,     y01 = (int16_t)pos[r + 1][c].y;

                    // Render intact half-triangles per micro-cell (gap-free, fast scanline fill)
                    if (triTopRight)   eng.fillTriangle(x00, y00, x10, y10, x11, y11, shadedCol);
                    if (triBottomLeft) eng.fillTriangle(x00, y00, x11, y11, x01, y01, shadedCol);
                } else {
                    if (linkRight[r][c])
                        eng.drawLine((int)pos[r][c].x, (int)pos[r][c].y, (int)pos[r][c + 1].x, (int)pos[r][c + 1].y, clothColor1);
                    if (linkDown[r][c])
                        eng.drawLine((int)pos[r][c].x, (int)pos[r][c].y, (int)pos[r + 1][c].x, (int)pos[r + 1][c].y, clothColor1);
                }
            }
        }

        // Draw golden curtain rings at top pins & fine silk fringe tassels along the bottom hem
        for (int c = 0; c < COLS; c++) {
            if (pinned[0][c]) {
                eng.fillCircle((int16_t)pos[0][c].x, (int16_t)pos[0][c].y, 4, Colors565::Gold);
                eng.drawCircle((int16_t)pos[0][c].x, (int16_t)pos[0][c].y, 4, Colors565::White);
            }
            if ((c % 2 == 0) && linkDown[ROWS - 2][c]) {
                int16_t bx = (int16_t)pos[ROWS - 1][c].x;
                int16_t by = (int16_t)pos[ROWS - 1][c].y;
                eng.drawLine(bx, by, bx, by + 7, Colors565::Gold);
                eng.drawPixel(bx, by + 8, Colors565::White);
            }
        }
    }
};

// ============================================================================
// 6. 1D VERLET ROPE / FLOWING NINJA SCARF (CONTIGUOUS RIBBON MESH)
// ============================================================================
class VerletRope2D
{
public:
    static constexpr int NODES = 16;
    Vec2 pos[NODES];
    Vec2 oldPos[NODES];
    float segLen;
    uint16_t color;

    void init(float headX, float headY, float segmentLen = 10.0f, uint16_t col = Colors565::Crimson) {
        segLen = segmentLen;
        color = col;
        for (int i = 0; i < NODES; i++) {
            pos[i] = Vec2(headX - i * segLen, headY);
            oldPos[i] = pos[i];
        }
    }

    void update(float headX, float headY, float dt, float windX = -280.0f, float gravityY = 220.0f) {
        (void)dt;
        const float fixedDt2 = 0.016f * 0.016f;
        pos[0] = Vec2(headX, headY);
        float t = millis() * 0.0085f;

        for (int i = 1; i < NODES; i++) {
            Vec2 v = (pos[i] - oldPos[i]) * 0.955f;
            oldPos[i] = pos[i];
            float flutter = sinf(t - i * 0.55f) * 260.0f;
            pos[i].x += v.x + windX * fixedDt2;
            pos[i].y += v.y + (gravityY + flutter) * fixedDt2;
        }

        for (int iter = 0; iter < 5; iter++) {
            pos[0] = Vec2(headX, headY);
            for (int i = 0; i < NODES - 1; i++) {
                Vec2 d = pos[i + 1] - pos[i];
                float len = d.length();
                if (len > 0.001f) {
                    Vec2 corr = d * ((len - segLen) / len);
                    if (i == 0) {
                        pos[i + 1] -= corr;
                    } else {
                        pos[i]     += corr * 0.5f;
                        pos[i + 1] -= corr * 0.5f;
                    }
                }
            }
        }
    }

    void drawScarf(RGBGameEngine &eng, float baseThickness = 13.0f) {
        Vec2 prevTop = pos[0], prevBot = pos[0];
        for (int i = 0; i < NODES - 1; i++) {
            float t = 1.0f - (float)i / (NODES - 1);
            float halfW = max(1.5f, baseThickness * 0.5f * (0.35f + 0.65f * sinf(t * 3.14159265f * 0.85f)));
            Vec2 d = pos[i + 1] - pos[i];
            float len = d.length();
            Vec2 n = (len > 0.01f) ? Vec2(-d.y / len, d.x / len) : Vec2(0.0f, 1.0f);

            Vec2 curTop = pos[i + 1] + n * halfW;
            Vec2 curBot = pos[i + 1] - n * halfW;
            if (i == 0) {
                prevTop = pos[0] + n * (baseThickness * 0.4f);
                prevBot = pos[0] - n * (baseThickness * 0.4f);
            }

            // Shade scarf folds based on vertical slope
            uint8_t shade = (uint8_t)constrain((int)(185 + d.y * 6.0f), 110, 255);
            uint16_t segCol = Color::scaleBrightness565(color, shade);
            eng.fillQuad((int16_t)prevTop.x, (int16_t)prevTop.y, (int16_t)curTop.x, (int16_t)curTop.y,
                         (int16_t)curBot.x,  (int16_t)curBot.y,  (int16_t)prevBot.x, (int16_t)prevBot.y, segCol);
            eng.drawLine((int)prevTop.x, (int)prevTop.y, (int)curTop.x, (int)curTop.y, Colors565::Gold);

            prevTop = curTop;
            prevBot = curBot;
        }
    }
};

// ============================================================================
// 7. 2-BONE INVERSE KINEMATICS (IK) SOLVER & INTERACTIVE WATER WAVES
// ============================================================================
namespace IK2D
{
    // Analytic 2-bone IK solver using Law of Cosines
    inline Vec2 solveTwoBoneIK(Vec2 root, Vec2 target, float len1, float len2, bool bendPositive = true) {
        Vec2 d = target - root;
        float dist = d.length();
        float maxReach = len1 + len2 - 0.1f;
        float minReach = fabsf(len1 - len2) + 0.1f;
        dist = constrain(dist, minReach, maxReach);

        float baseAngle = atan2f(d.y, d.x);
        float cosAngle1 = (len1 * len1 + dist * dist - len2 * len2) / (2.0f * len1 * dist);
        cosAngle1 = constrain(cosAngle1, -1.0f, 1.0f);
        float angle1 = acosf(cosAngle1);

        float jointAngle = bendPositive ? (baseAngle + angle1) : (baseAngle - angle1);
        return root + Vec2(cosf(jointAngle) * len1, sinf(jointAngle) * len1);
    }
}

class WaterSurface2D
{
public:
    static constexpr int COLUMNS = 41;
    float heightOffset[COLUMNS];
    float velocity[COLUMNS];
    float xStart, width, baseY, depth;

    void init(float x, float y, float w, float d) {
        xStart = x; baseY = y; width = w; depth = d;
        for (int i = 0; i < COLUMNS; i++) {
            heightOffset[i] = 0.0f;
            velocity[i] = 0.0f;
        }
    }

    void splash(float splashX, float force = 220.0f) {
        int idx = (int)(((splashX - xStart) / width) * (COLUMNS - 1));
        for (int i = max(1, idx - 2); i <= min(COLUMNS - 2, idx + 2); i++) {
            velocity[i] += force * (1.0f - abs(i - idx) * 0.3f);
        }
    }

    void update(float dt) {
        const float k = 85.0f, damp = 0.97f, spread = 140.0f;
        for (int i = 1; i < COLUMNS - 1; i++) {
            float accel = -k * heightOffset[i];
            velocity[i] += accel * dt;
            velocity[i] *= damp;
            heightOffset[i] += velocity[i] * dt;
        }
        for (int i = 1; i < COLUMNS - 1; i++) {
            float neighborDelta = (heightOffset[i - 1] + heightOffset[i + 1]) - 2.0f * heightOffset[i];
            velocity[i] += neighborDelta * spread * dt;
        }
    }

    void draw(RGBGameEngine &eng, uint16_t waterColor = Colors565::DodgerBlue, uint8_t alpha = 150) {
        float colW = width / (COLUMNS - 1);
        int16_t bottomY = (int16_t)(baseY + depth);

        for (int i = 0; i < COLUMNS - 1; i++) {
            int16_t x0 = (int16_t)(xStart + i * colW);
            int16_t x1 = (int16_t)(xStart + (i + 1) * colW);
            int16_t y0 = (int16_t)(baseY + heightOffset[i]);
            int16_t y1 = (int16_t)(baseY + heightOffset[i + 1]);
            int16_t ym = (y0 + y1) / 2;

            eng.fillRectAlpha(x0, ym, max(1, x1 - x0 + 1), bottomY - ym, waterColor, alpha);
            eng.drawThickLine(x0, y0, x1, y1, 3, Colors565::ElectricBlue);
        }
    }
};

// ============================================================================
// 8. SWORD SLASH / MOTION RIBBON TRAIL, WEATHER & UI WIDGETS
// ============================================================================
class RibbonTrail2D
{
public:
    static constexpr int MAX_POINTS = 18;
    Vec2 pts[MAX_POINTS];
    int count;

    RibbonTrail2D() : count(0) {}

    void addPoint(float x, float y) {
        if (count < MAX_POINTS) {
            pts[count++] = Vec2(x, y);
        } else {
            for (int i = 0; i < MAX_POINTS - 1; i++) pts[i] = pts[i + 1];
            pts[MAX_POINTS - 1] = Vec2(x, y);
        }
    }

    void draw(RGBGameEngine &eng, float maxThickness, uint16_t tipColor, uint16_t tailColor) {
        if (count < 2) return;
        for (int i = 0; i < count - 1; i++) {
            float t = (float)(i + 1) / count;
            int16_t thick = max(1, (int)(maxThickness * t));
            uint16_t col = Color::lerp565(tailColor, tipColor, (uint8_t)(t * 255));
            eng.drawThickLine((int)pts[i].x, (int)pts[i].y, (int)pts[i + 1].x, (int)pts[i + 1].y, thick, col, true);
        }
    }
};

enum class WeatherType : uint8_t { NONE = 0, RAIN, SNOW, SAKURA_PETALS };

class WeatherSystem2D
{
public:
    static constexpr int MAX_DROPS = 70;
    struct Drop { float x, y, vx, vy, size; };
    Drop drops[MAX_DROPS];
    WeatherType type;

    void init(WeatherType wt = WeatherType::RAIN) {
        type = wt;
        for (int i = 0; i < MAX_DROPS; i++) respawn(i, true);
    }

    void respawn(int i, bool randomY = false) {
        drops[i].x = (float)random(0, 800);
        drops[i].y = randomY ? (float)random(0, 480) : (float)random(-40, 0);
        if (type == WeatherType::RAIN) {
            drops[i].vx = -120.0f;
            drops[i].vy = (float)random(520, 760);
            drops[i].size = (float)random(10, 20);
        } else if (type == WeatherType::SNOW) {
            drops[i].vx = (float)random(-40, 40);
            drops[i].vy = (float)random(70, 140);
            drops[i].size = (float)random(2, 5);
        } else {
            drops[i].vx = (float)random(-90, -30);
            drops[i].vy = (float)random(80, 150);
            drops[i].size = (float)random(4, 7);
        }
    }

    void updateAndDraw(RGBGameEngine &eng, float dt) {
        if (type == WeatherType::NONE) return;
        float t = millis() * 0.004f;
        for (int i = 0; i < MAX_DROPS; i++) {
            Drop &d = drops[i];
            d.x += (d.vx + ((type != WeatherType::RAIN) ? sinf(t + i) * 35.0f : 0.0f)) * dt;
            d.y += d.vy * dt;
            if (d.y > 485 || d.x < -20 || d.x > 820) respawn(i, false);

            if (type == WeatherType::RAIN) {
                eng.drawLine((int)d.x, (int)d.y, (int)(d.x - 4), (int)(d.y + d.size), Colors565::SkyBlue);
            } else if (type == WeatherType::SNOW) {
                eng.fillCircle((int16_t)d.x, (int16_t)d.y, (int16_t)d.size, Colors565::White);
            } else {
                eng.fillEllipse((int16_t)d.x, (int16_t)d.y, (int16_t)d.size, (int16_t)(d.size * 0.6f), Colors565::SakuraPink);
            }
        }
    }
};

namespace UIWidgets
{
    inline void drawHealthBar(RGBGameEngine &eng, int16_t x, int16_t y, int16_t w, int16_t h, float currentHP, float ghostHP, float maxHP, const char *label = "HP") {
        eng.fillRoundRect(x, y, w, h, 5, Colors565::Charcoal);
        int16_t gw = (int16_t)((constrain(ghostHP, 0.0f, maxHP) / maxHP) * (w - 4));
        int16_t cw = (int16_t)((constrain(currentHP, 0.0f, maxHP) / maxHP) * (w - 4));
        if (gw > cw) eng.fillRoundRect(x + 2, y + 2, gw, h - 4, 4, Colors565::Crimson);
        uint16_t barCol = (currentHP > maxHP * 0.5f) ? Colors565::Emerald : ((currentHP > maxHP * 0.25f) ? Colors565::Amber : Colors565::NeonRed);
        if (cw > 0) eng.fillRoundRect(x + 2, y + 2, cw, h - 4, 4, barCol);
        eng.drawRoundRect(x, y, w, h, 5, Colors565::White);
        if (label) eng.drawTextShadow(x + 8, y + (h - 14) / 2, label, Colors565::White, Colors565::Black, 2, 1);
    }

    inline void drawCircularGauge(RGBGameEngine &eng, int16_t cx, int16_t cy, int16_t radius, float value01, uint16_t activeColor, const char *label = "") {
        value01 = constrain(value01, 0.0f, 1.0f);
        eng.fillArcRing(cx, cy, radius - 10, radius, 135.0f, 405.0f, Colors565::DarkSlateGray);
        if (value01 > 0.01f) {
            eng.fillArcRing(cx, cy, radius - 10, radius, 135.0f, 135.0f + value01 * 270.0f, activeColor);
        }
        eng.drawCircle(cx, cy, radius, Colors565::Silver);
        if (label && label[0]) {
            eng.drawText(cx - (strlen(label) * 6), cy + radius - 6, label, Colors565::White, 2);
        }
    }
}

// ============================================================================
// 9. UNIVERSAL SHAPE-DRIVEN PARTICLE PHYSICS & BEHAVIOR ENGINE
// ============================================================================

enum class EmitterShape : uint8_t
{
    POINT = 0,   // Single coordinate (x, y)
    LINE,        // Line segment from (x, y) to (x2, y2)
    CIRCLE,      // Circle with center (x, y) and radius (param1)
    RING,        // Ring with center (x, y), innerRadius (param1), outerRadius (param2)
    RECTANGLE,   // Rectangle at (x, y) with width (param1), height (param2)
    TRIANGLE     // Triangle with vertices (x,y), (x2,y2), (x3,y3)
};

enum class EmitRegionMode : uint8_t
{
    SURFACE_AREA = 0, // Uniformly anywhere inside the 2D surface area of the shape
    EDGE_PERIMETER,   // Strictly along the outer perimeter/edges of the shape
    CENTER_POINT,     // From the geometric centroid of the shape
    VERTICES_ONLY     // Only from the corner vertices of the shape
};

enum class ParticleBehavior : uint8_t
{
    REALISTIC_FIRE = 0, // Volumetric buoyant thermal fire plume (White-hot core -> Yellow -> Orange -> Red -> Smoke + Sparks!)
    PARALLEL,           // Uniform parallel flow (e.g. straight vertical rainfall, laser stream)
    PARALLEL_WIND,      // Parallel stream + directional wind field with sinusoidal gusts
    SLIGHTLY_CHAOTIC,   // Rising glowing embers / turbulent drift
    TOTALLY_CHAOTIC,    // 360-deg random launch + continuous Brownian/turbulent acceleration (sparks, plasma)
    OUTWARD_NORMAL,     // Emits perpendicular/outward from the shape's surface/edge normal
    INWARD_IMPLODE,     // Sucked inward from the shape's boundary toward its center (energy charge)
    VORTEX_SWIRL,       // Tangential orbital swirl + spiral pull around the emitter center (tornado/portal)
    HARMONIC_WAVE,      // Travels along base direction while oscillating in a sine wave
    GRAVITY_BOUNCE,     // Parabolic gravity fountain that bounces off the floor
    HOMING_ATTRACTOR    // Launches chaotically then curves/steers toward (attractorX, attractorY)
};

enum class ParticleRenderStyle : uint8_t
{
    FILLED_CIRCLE = 0,
    VELOCITY_STREAK,    // Motion-blur line aligned with velocity (rain, fast sparks)
    ADDITIVE_GLOW,      // Translucent glowing orb
    FIRE_PLUME,         // Multi-stage Blackbody thermal flame tongue + crackling embers + smoke
    PIXEL_DOT
};

struct Particle2D
{
    Vec2 pos;
    Vec2 vel;
    Vec2 spawnOrigin;
    float age;
    float life;
    float phase;        // Per-particle random phase for harmonic wave & flame flicker
    float size;
    float initialSize;
    uint16_t startColor;
    uint16_t endColor;
    bool isSpark;       // True for fast crackling embers flying out of the fire
    bool active;
};

class ParticleSystem2D
{
public:
    static constexpr int MAX_PARTICLES = 140;
    Particle2D pool[MAX_PARTICLES];

    // --- Emitter Shape & Region Configuration ---
    EmitterShape shape;
    EmitRegionMode regionMode;
    ParticleBehavior behavior;
    ParticleRenderStyle renderStyle;

    float x, y;           // Point, Center of Circle/Ring, Top-Left of Rect, or Vertex 1 of Line/Triangle
    float x2, y2;         // End of Line, or Vertex 2 of Triangle
    float x3, y3;         // Vertex 3 of Triangle
    float param1, param2; // Circle radius, Ring (innerR, outerR), or Rect (width, height)

    // --- Motion & Behavior Physics Parameters ---
    float baseAngleDeg;   // Direction for PARALLEL / PARALLEL_WIND / SLIGHTLY_CHAOTIC (90 = straight down, -90 = up)
    float baseSpeed;      // Initial launch speed (px/s)
    float windX, windY;   // Wind vector for PARALLEL_WIND (px/s^2)
    float windGustFreq;   // Gust oscillation frequency
    float chaosStrength;  // Turbulence acceleration magnitude (px/s^2)
    float gravityY;       // Gravity for GRAVITY_BOUNCE
    float floorY;         // Bounce floor Y coordinate
    float attractorX, attractorY; // Target for HOMING_ATTRACTOR

    // --- Visual & Spawn Lifecycle Parameters ---
    float spawnRate;      // Particles spawned per second
    float spawnAccum;
    float minLife, maxLife;
    float startSize, endSize;
    uint16_t startColor, endColor;
    bool drawEmitterWireframe;

    ParticleSystem2D() {
        shape = EmitterShape::LINE;
        regionMode = EmitRegionMode::EDGE_PERIMETER;
        behavior = ParticleBehavior::PARALLEL_WIND;
        renderStyle = ParticleRenderStyle::VELOCITY_STREAK;

        x = 100; y = 70; x2 = 700; y2 = 70; x3 = 400; y3 = 200;
        param1 = 90; param2 = 120;

        baseAngleDeg = 90.0f; // Downward
        baseSpeed = 320.0f;
        windX = -180.0f; windY = 0.0f;
        windGustFreq = 2.5f;
        chaosStrength = 260.0f;
        gravityY = 850.0f;
        floorY = 445.0f;
        attractorX = 650.0f; attractorY = 240.0f;

        spawnRate = 85.0f;
        spawnAccum = 0.0f;
        minLife = 0.9f; maxLife = 1.8f;
        startSize = 4.0f; endSize = 1.0f;
        startColor = Colors565::NeonCyan;
        endColor = Colors565::RoyalBlue;
        drawEmitterWireframe = true;

        clear();
    }

    void clear() {
        for (int i = 0; i < MAX_PARTICLES; i++) pool[i].active = false;
    }

    // --- Helper Setters for Emitter Shapes ---
    void setPointEmitter(float px, float py) {
        shape = EmitterShape::POINT; x = px; y = py;
    }
    void setLineEmitter(float x0, float y0, float x1, float y1) {
        shape = EmitterShape::LINE; x = x0; y = y0; x2 = x1; y2 = y1;
    }
    void setCircleEmitter(float cx, float cy, float radius, EmitRegionMode mode = EmitRegionMode::EDGE_PERIMETER) {
        shape = EmitterShape::CIRCLE; x = cx; y = cy; param1 = radius; regionMode = mode;
    }
    void setRingEmitter(float cx, float cy, float innerR, float outerR, EmitRegionMode mode = EmitRegionMode::SURFACE_AREA) {
        shape = EmitterShape::RING; x = cx; y = cy; param1 = innerR; param2 = outerR; regionMode = mode;
    }
    void setRectEmitter(float rx, float ry, float rw, float rh, EmitRegionMode mode = EmitRegionMode::EDGE_PERIMETER) {
        shape = EmitterShape::RECTANGLE; x = rx; y = ry; param1 = rw; param2 = rh; regionMode = mode;
    }
    void setTriangleEmitter(float ax, float ay, float bx, float by, float cx, float cy, EmitRegionMode mode = EmitRegionMode::EDGE_PERIMETER) {
        shape = EmitterShape::TRIANGLE; x = ax; y = ay; x2 = bx; y2 = by; x3 = cx; y3 = cy; regionMode = mode;
    }

    // ------------------------------------------------------------------------
    // Compute Exact Spawn Point & Outward Surface Normal on Any Shape
    // ------------------------------------------------------------------------
    void sampleShapeSpawn(Vec2 &outPos, Vec2 &outNormal, Vec2 &outCenter) const
    {
        float u = (float)random(0, 10001) * 0.0001f;
        float v = (float)random(0, 10001) * 0.0001f;

        switch (shape)
        {
            case EmitterShape::POINT: {
                outCenter = Vec2(x, y);
                outPos = outCenter;
                float a = u * 6.2831853f;
                outNormal = Vec2(cosf(a), sinf(a));
                break;
            }
            case EmitterShape::LINE: {
                outCenter = Vec2((x + x2) * 0.5f, (y + y2) * 0.5f);
                if (regionMode == EmitRegionMode::CENTER_POINT) {
                    outPos = outCenter;
                } else if (regionMode == EmitRegionMode::VERTICES_ONLY) {
                    outPos = (u < 0.5f) ? Vec2(x, y) : Vec2(x2, y2);
                } else {
                    outPos = Vec2::lerp(Vec2(x, y), Vec2(x2, y2), u);
                }
                Vec2 tangent = (Vec2(x2, y2) - Vec2(x, y)).normalized();
                outNormal = Vec2(-tangent.y, tangent.x);
                break;
            }
            case EmitterShape::CIRCLE: {
                outCenter = Vec2(x, y);
                float a = u * 6.2831853f;
                Vec2 dir(cosf(a), sinf(a));
                outNormal = dir;
                if (regionMode == EmitRegionMode::CENTER_POINT) {
                    outPos = outCenter;
                } else if (regionMode == EmitRegionMode::EDGE_PERIMETER || regionMode == EmitRegionMode::VERTICES_ONLY) {
                    outPos = outCenter + dir * param1;
                } else {
                    // Uniform disk area sampling using sqrt(v)
                    outPos = outCenter + dir * (param1 * sqrtf(v));
                }
                break;
            }
            case EmitterShape::RING: {
                outCenter = Vec2(x, y);
                float a = u * 6.2831853f;
                Vec2 dir(cosf(a), sinf(a));
                outNormal = dir;
                if (regionMode == EmitRegionMode::EDGE_PERIMETER) {
                    float r = (v < 0.5f) ? param1 : param2;
                    outPos = outCenter + dir * r;
                } else {
                    float r = param1 + (param2 - param1) * v;
                    outPos = outCenter + dir * r;
                }
                break;
            }
            case EmitterShape::RECTANGLE: {
                outCenter = Vec2(x + param1 * 0.5f, y + param2 * 0.5f);
                if (regionMode == EmitRegionMode::CENTER_POINT) {
                    outPos = outCenter;
                    outNormal = Vec2(0, -1);
                } else if (regionMode == EmitRegionMode::VERTICES_ONLY) {
                    int corner = random(0, 4);
                    outPos = Vec2(x + ((corner & 1) ? param1 : 0), y + ((corner & 2) ? param2 : 0));
                    outNormal = (outPos - outCenter).normalized();
                } else if (regionMode == EmitRegionMode::EDGE_PERIMETER) {
                    float perim = 2.0f * (param1 + param2);
                    float d = u * perim;
                    if (d < param1) {
                        outPos = Vec2(x + d, y); outNormal = Vec2(0, -1);
                    } else if (d < param1 + param2) {
                        outPos = Vec2(x + param1, y + (d - param1)); outNormal = Vec2(1, 0);
                    } else if (d < 2.0f * param1 + param2) {
                        outPos = Vec2(x + param1 - (d - (param1 + param2)), y + param2); outNormal = Vec2(0, 1);
                    } else {
                        outPos = Vec2(x, y + param2 - (d - (2.0f * param1 + param2))); outNormal = Vec2(-1, 0);
                    }
                } else {
                    // Full interior surface area
                    outPos = Vec2(x + u * param1, y + v * param2);
                    outNormal = (outPos - outCenter).normalized();
                }
                break;
            }
            case EmitterShape::TRIANGLE: {
                Vec2 A(x, y), B(x2, y2), C(x3, y3);
                outCenter = (A + B + C) * (1.0f / 3.0f);
                if (regionMode == EmitRegionMode::CENTER_POINT) {
                    outPos = outCenter;
                } else if (regionMode == EmitRegionMode::VERTICES_ONLY) {
                    int k = random(0, 3);
                    outPos = (k == 0) ? A : ((k == 1) ? B : C);
                } else if (regionMode == EmitRegionMode::EDGE_PERIMETER) {
                    int edge = random(0, 3);
                    if (edge == 0) outPos = Vec2::lerp(A, B, u);
                    else if (edge == 1) outPos = Vec2::lerp(B, C, u);
                    else outPos = Vec2::lerp(C, A, u);
                } else {
                    // Uniform triangle interior area sampling
                    if (u + v > 1.0f) { u = 1.0f - u; v = 1.0f - v; }
                    outPos = A + (B - A) * u + (C - A) * v;
                }
                outNormal = (outPos - outCenter).normalized();
                break;
            }
        }
    }

    // ------------------------------------------------------------------------
    // Physical Blackbody Thermal Flame Palette (0.0 = White-Hot Base -> 1.0 = Smoke Tip)
    // ------------------------------------------------------------------------
    static inline uint16_t getFireThermalColor565(float lifeRatio)
    {
        if (lifeRatio < 0.18f) {
            // 0.00..0.18: Blinding White-Hot Core -> Incandescent Lemon Yellow
            uint8_t t = (uint8_t)((lifeRatio / 0.18f) * 255.0f);
            return Color::lerp565(Colors565::White, Colors565::ElectricYellow, t);
        } else if (lifeRatio < 0.46f) {
            // 0.18..0.46: Lemon Yellow -> Blazing Flame Orange
            uint8_t t = (uint8_t)(((lifeRatio - 0.18f) / 0.28f) * 255.0f);
            return Color::lerp565(Colors565::ElectricYellow, Colors565::NeonOrange, t);
        } else if (lifeRatio < 0.76f) {
            // 0.46..0.76: Flame Orange -> Deep Crimson / Scarlet Tongue
            uint8_t t = (uint8_t)(((lifeRatio - 0.46f) / 0.30f) * 255.0f);
            return Color::lerp565(Colors565::NeonOrange, Colors565::Crimson, t);
        } else {
            // 0.76..1.00: Deep Crimson -> Dark Sooty Smoke
            uint8_t t = (uint8_t)(constrain((lifeRatio - 0.76f) / 0.24f, 0.0f, 1.0f) * 255.0f);
            return Color::lerp565(Colors565::Crimson, Colors565::Charcoal, t);
        }
    }

    // ------------------------------------------------------------------------
    // Spawn a Single Particle with Initial Velocity Matched to Behavior Pattern
    // ------------------------------------------------------------------------
    void spawnOne()
    {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (pool[i].active) continue;

            Particle2D &p = pool[i];
            p.active = true;
            p.age = 0.0f;
            p.life = minLife + ((float)random(0, 1001) * 0.001f) * (maxLife - minLife);
            p.phase = (float)random(0, 628) * 0.01f;
            p.size = startSize;
            p.initialSize = startSize;
            p.startColor = startColor;
            p.endColor = endColor;
            p.isSpark = false;

            Vec2 normal, center;
            sampleShapeSpawn(p.pos, normal, center);
            p.spawnOrigin = center;

            float baseRad = baseAngleDeg * 0.0174532925f;
            Vec2 baseDir(cosf(baseRad), sinf(baseRad));
            float speedJitter = baseSpeed * (0.85f + 0.30f * ((float)random(0, 100) * 0.01f));

            switch (behavior)
            {
                case ParticleBehavior::REALISTIC_FIRE: {
                    // 16% of particles are fast crackling embers/sparks; 84% are volumetric flame tongues
                    p.isSpark = (random(0, 100) < 16);
                    if (p.isSpark) {
                        p.initialSize = 2.0f;
                        p.size = 2.0f;
                        p.life *= 1.15f;
                        p.vel = Vec2((float)random(-95, 96), -baseSpeed * (1.25f + (random(0, 60) * 0.01f)));
                    } else {
                        // Cluster flame tongues denser near the center of the hearth
                        float distFromCenter = fabsf(p.pos.x - center.x);
                        float widthScale = constrain(1.1f - distFromCenter / 120.0f, 0.55f, 1.1f);
                        p.initialSize = startSize * (0.8f + 0.45f * ((float)random(0, 100) * 0.01f)) * widthScale;
                        p.size = p.initialSize;
                        p.life *= widthScale;
                        p.vel = Vec2((float)random(-28, 29), -baseSpeed * (0.55f + 0.45f * widthScale));
                    }
                    break;
                }

                case ParticleBehavior::PARALLEL:
                case ParticleBehavior::PARALLEL_WIND:
                    p.vel = baseDir * speedJitter;
                    break;

                case ParticleBehavior::SLIGHTLY_CHAOTIC: {
                    float spreadRad = baseRad + ((float)random(-31, 32) * 0.01f);
                    p.vel = Vec2(cosf(spreadRad), sinf(spreadRad)) * speedJitter;
                    break;
                }

                case ParticleBehavior::TOTALLY_CHAOTIC:
                case ParticleBehavior::HOMING_ATTRACTOR: {
                    float randRad = (float)random(0, 628) * 0.01f;
                    float randSpd = baseSpeed * (0.4f + 1.1f * ((float)random(0, 100) * 0.01f));
                    p.vel = Vec2(cosf(randRad), sinf(randRad)) * randSpd;
                    break;
                }

                case ParticleBehavior::OUTWARD_NORMAL:
                    p.vel = normal * speedJitter;
                    break;

                case ParticleBehavior::INWARD_IMPLODE: {
                    Vec2 toCenter = (center - p.pos).normalized();
                    p.vel = toCenter * speedJitter;
                    break;
                }

                case ParticleBehavior::VORTEX_SWIRL: {
                    Vec2 radial = (p.pos - center).normalized();
                    Vec2 tangent(-radial.y, radial.x);
                    p.vel = tangent * speedJitter;
                    break;
                }

                case ParticleBehavior::HARMONIC_WAVE:
                    p.vel = baseDir * speedJitter;
                    break;

                case ParticleBehavior::GRAVITY_BOUNCE: {
                    float spreadRad = baseRad + ((float)random(-45, 46) * 0.01f);
                    p.vel = Vec2(cosf(spreadRad), sinf(spreadRad)) * speedJitter;
                    break;
                }
            }
            return;
        }
    }

    // ------------------------------------------------------------------------
    // Update All Active Particles with Behavior Forces & Kinematics
    // ------------------------------------------------------------------------
    void update(float dt)
    {
        spawnAccum += spawnRate * dt;
        while (spawnAccum >= 1.0f) {
            spawnOne();
            spawnAccum -= 1.0f;
        }

        float t = millis() * 0.001f;
        float gust = sinf(t * windGustFreq);

        for (int i = 0; i < MAX_PARTICLES; i++) {
            Particle2D &p = pool[i];
            if (!p.active) continue;

            p.age += dt;
            if (p.age >= p.life) {
                p.active = false;
                continue;
            }

            float lifeRatio = p.age / p.life;

            switch (behavior)
            {
                case ParticleBehavior::REALISTIC_FIRE: {
                    // 1. Thermal Buoyancy: hot gases accelerate upward as they rise!
                    p.vel.y -= 280.0f * dt;

                    if (p.isSpark) {
                        // Crackling sparks dance rapidly in turbulent eddies
                        float sparkFlicker = sinf(t * 16.0f + p.phase * 3.0f) * 380.0f;
                        p.vel.x += (sparkFlicker + windX * 0.35f) * dt;
                        p.size = max(1.0f, 2.5f * (1.0f - lifeRatio));
                    } else {
                        // 2. Venturi Flame Necking: cool air pinches rising flame tongues inward toward the center axis!
                        float dxToAxis = p.spawnOrigin.x - p.pos.x;
                        float pinchForce = dxToAxis * (3.8f + 4.5f * lifeRatio);

                        // 3. Sinuous Rayleigh-Taylor Flame Flicker Waves traveling upward
                        float wave = sinf(t * 9.5f - p.pos.y * 0.048f + p.phase) * (165.0f * (0.35f + 0.85f * lifeRatio));
                        p.vel.x += (pinchForce + wave + windX * 0.25f * lifeRatio) * dt;
                        p.vel.x *= 0.95f;

                        // Flame puffs swell slightly near the base, taper to sharp tongues in mid-flame, and expand slightly as smoke at the tip
                        if (lifeRatio < 0.75f) {
                            p.size = max(2.0f, p.initialSize * (1.05f - 0.82f * (lifeRatio / 0.75f)));
                        } else {
                            p.size = max(4.0f, p.initialSize * (0.28f + 0.35f * ((lifeRatio - 0.75f) / 0.25f)));
                        }
                    }
                    p.pos += p.vel * dt;
                    continue;
                }

                case ParticleBehavior::PARALLEL:
                    break;

                case ParticleBehavior::PARALLEL_WIND: {
                    float curWindX = windX * (0.7f + 0.5f * gust);
                    p.vel.x += curWindX * dt;
                    p.vel.y += windY * dt;
                    break;
                }

                case ParticleBehavior::SLIGHTLY_CHAOTIC: {
                    float turbX = sinf(t * 5.0f + p.pos.y * 0.045f + p.phase) * chaosStrength * 0.55f;
                    float turbY = cosf(t * 4.2f + p.pos.x * 0.045f + p.phase) * chaosStrength * 0.35f - 140.0f;
                    p.vel.x += turbX * dt;
                    p.vel.y += turbY * dt;
                    break;
                }

                case ParticleBehavior::TOTALLY_CHAOTIC: {
                    p.vel.x += (float)random(-100, 101) * 0.01f * chaosStrength * 4.0f * dt;
                    p.vel.y += (float)random(-100, 101) * 0.01f * chaosStrength * 4.0f * dt;
                    break;
                }

                case ParticleBehavior::OUTWARD_NORMAL:
                case ParticleBehavior::INWARD_IMPLODE:
                    break;

                case ParticleBehavior::VORTEX_SWIRL: {
                    Vec2 d = p.pos - p.spawnOrigin;
                    float dist = max(6.0f, d.length());
                    Vec2 radial = d / dist;
                    Vec2 tangent(-radial.y, radial.x);
                    Vec2 accel = tangent * 260.0f - radial * (baseSpeed * baseSpeed / dist);
                    p.vel += accel * dt;
                    break;
                }

                case ParticleBehavior::HARMONIC_WAVE: {
                    float baseRad = baseAngleDeg * 0.0174532925f;
                    Vec2 forward(cosf(baseRad), sinf(baseRad));
                    Vec2 side(-forward.y, forward.x);
                    float waveVel = cosf(p.age * 10.0f + p.phase) * 180.0f;
                    p.vel = forward * baseSpeed + side * waveVel;
                    break;
                }

                case ParticleBehavior::GRAVITY_BOUNCE: {
                    p.vel.y += gravityY * dt;
                    if (p.pos.y + p.size > floorY && p.vel.y > 0.0f) {
                        p.pos.y = floorY - p.size;
                        p.vel.y = -p.vel.y * 0.68f;
                        p.vel.x *= 0.88f;
                    }
                    break;
                }

                case ParticleBehavior::HOMING_ATTRACTOR: {
                    Vec2 toTarget = Vec2(attractorX, attractorY) - p.pos;
                    float dist = max(5.0f, toTarget.length());
                    Vec2 desired = (toTarget / dist) * (baseSpeed * 1.4f);
                    p.vel += (desired - p.vel) * (5.5f * dt);
                    break;
                }
            }

            p.pos += p.vel * dt;
            p.size = startSize + (endSize - startSize) * lifeRatio;
        }
    }

    // ------------------------------------------------------------------------
    // Render Emitter Wireframe & All Active Particles
    // ------------------------------------------------------------------------
    void draw(RGBGameEngine &eng)
    {
        // For REALISTIC_FIRE, draw a warm flickering hearth glow & glowing fuel bed at the emitter base!
        if (behavior == ParticleBehavior::REALISTIC_FIRE || renderStyle == ParticleRenderStyle::FIRE_PLUME) {
            int16_t cx = (shape == EmitterShape::LINE) ? (int16_t)((x + x2) * 0.5f) : (int16_t)x;
            int16_t cy = (shape == EmitterShape::LINE) ? (int16_t)((y + y2) * 0.5f) : (int16_t)y;
            int16_t baseW = (shape == EmitterShape::LINE) ? (int16_t)(fabsf(x2 - x) * 0.5f) : 65;
            uint8_t flicker = (uint8_t)random(55, 85);
            eng.fillCircleBlend(cx, cy - 35, baseW + 45, Colors565::Crimson, flicker, BlendMode::ADDITIVE);
            eng.fillRoundRect(cx - baseW, cy + 4, baseW * 2, 14, 5, Colors565::SaddleBrown);
            eng.fillRoundRectBlend(cx - baseW + 6, cy - 2, (baseW - 6) * 2, 10, 4, Colors565::NeonOrange, 210, BlendMode::ADDITIVE);
        } else if (drawEmitterWireframe) {
            uint16_t wfCol = Colors565::DimGray;
            switch (shape) {
                case EmitterShape::POINT:
                    eng.drawCrosshair((int16_t)x, (int16_t)y, 10, wfCol); break;
                case EmitterShape::LINE:
                    eng.drawDashedLine((int)x, (int)y, (int)x2, (int)y2, 8, 6, wfCol); break;
                case EmitterShape::CIRCLE:
                    eng.drawCircle((int16_t)x, (int16_t)y, (int16_t)param1, wfCol); break;
                case EmitterShape::RING:
                    eng.drawRing((int16_t)x, (int16_t)y, (int16_t)param1, (int16_t)param2, wfCol); break;
                case EmitterShape::RECTANGLE:
                    eng.drawRect((int)x, (int)y, (int)param1, (int)param2, wfCol); break;
                case EmitterShape::TRIANGLE:
                    eng.drawTriangle((int16_t)x, (int16_t)y, (int16_t)x2, (int16_t)y2, (int16_t)x3, (int16_t)y3, wfCol); break;
            }
            if (behavior == ParticleBehavior::HOMING_ATTRACTOR) {
                eng.drawCrosshair((int16_t)attractorX, (int16_t)attractorY, 14, Colors565::Gold);
            }
        }

        for (int i = 0; i < MAX_PARTICLES; i++) {
            const Particle2D &p = pool[i];
            if (!p.active) continue;

            float lifeRatio = constrain(p.age / p.life, 0.0f, 1.0f);
            int16_t px = (int16_t)p.pos.x;
            int16_t py = (int16_t)p.pos.y;
            int16_t r  = max((int16_t)1, (int16_t)p.size);

            if (renderStyle == ParticleRenderStyle::FIRE_PLUME || behavior == ParticleBehavior::REALISTIC_FIRE) {
                if (p.isSpark) {
                    // Crackling glowing ember streak
                    uint16_t sparkCol = (lifeRatio < 0.4f) ? Colors565::White : ((lifeRatio < 0.75f) ? Colors565::Gold : Colors565::NeonOrange);
                    int16_t tx = (int16_t)(p.pos.x - p.vel.x * 0.032f);
                    int16_t ty = (int16_t)(p.pos.y - p.vel.y * 0.032f);
                    eng.drawThickLine(px, py, tx, ty, 2, sparkCol, true);
                } else if (lifeRatio > 0.78f) {
                    // Rising dark smoke wisp at the top of the flame
                    uint8_t smokeAlpha = (uint8_t)((1.0f - (lifeRatio - 0.78f) / 0.22f) * 95.0f);
                    eng.fillCircleAlpha(px, py, r, Colors565::DarkSlateGray, smokeAlpha);
                } else {
                    // Volumetric thermal flame puff: outer additive aura + hotter inner tongue!
                    uint16_t outerCol = getFireThermalColor565(min(0.76f, lifeRatio + 0.14f));
                    uint16_t innerCol = getFireThermalColor565(max(0.0f,  lifeRatio - 0.12f));
                    uint8_t outerAlpha = (uint8_t)((1.0f - lifeRatio * 0.85f) * 145.0f);
                    uint8_t innerAlpha = (uint8_t)((1.0f - lifeRatio * 0.75f) * 195.0f);

                    eng.fillCircleBlend(px, py, r, outerCol, outerAlpha, BlendMode::ADDITIVE);
                    if (r >= 4) {
                        // Slightly vertically offset inner core gives realistic upward-licking flame tongue shape!
                        eng.fillCircleBlend(px, py + r / 5, (r * 3) / 5, innerCol, innerAlpha, BlendMode::ADDITIVE);
                    }
                }
                continue;
            }

            uint8_t t255 = (uint8_t)(lifeRatio * 255.0f);
            uint16_t col = Color::lerp565(p.startColor, p.endColor, t255);

            switch (renderStyle)
            {
                case ParticleRenderStyle::PIXEL_DOT:
                    eng.drawPixel(px, py, col);
                    break;
                case ParticleRenderStyle::FILLED_CIRCLE:
                    eng.fillCircle(px, py, r, col);
                    break;
                case ParticleRenderStyle::VELOCITY_STREAK: {
                    int16_t tailX = (int16_t)(p.pos.x - p.vel.x * 0.045f);
                    int16_t tailY = (int16_t)(p.pos.y - p.vel.y * 0.045f);
                    eng.drawThickLine(px, py, tailX, tailY, r, col, true);
                    break;
                }
                case ParticleRenderStyle::ADDITIVE_GLOW:
                default: {
                    uint8_t alpha = (uint8_t)((1.0f - lifeRatio) * 195.0f);
                    eng.fillCircleBlend(px, py, r * 2, col, alpha, BlendMode::ADDITIVE);
                    eng.fillCircleBlend(px, py, max((int16_t)1, r), Colors565::White, alpha / 2, BlendMode::ADDITIVE);
                    break;
                }
            }
        }
    }
};

// ============================================================================
// 12. CAMERA 2D (RUNTIME FAR-VIEW ZOOM-OUT, SHAKE & SLOW-MOTION)
// ============================================================================
class Camera2D
{
public:
    float zoom       = 1.0f;  // 1.0 = Normal Close-Up, 0.72 = Tactical, 0.50 = Far-View Story/Panorama
    float targetZoom = 1.0f;
    float anchorX    = 200.0f;
    float anchorY    = 432.0f;

    float shakeIntensity = 0.0f;
    float shakeTimer     = 0.0f;
    float shakeOffsetX   = 0.0f;
    float shakeOffsetY   = 0.0f;

    float slowMoScale    = 1.0f;
    float slowMoTimer    = 0.0f;

    void setTargetZoom(float z) {
        targetZoom = constrain(z, 0.42f, 1.35f);
    }

    void cycleZoomPreset() {
        if (targetZoom > 0.88f)      targetZoom = 0.68f; // Medium Tactical Zoom-Out
        else if (targetZoom > 0.58f) targetZoom = 0.48f; // Far Panorama Zoom-Out (See ~1650px ahead!)
        else                         targetZoom = 1.00f; // Normal Close-Up View
    }

    void triggerShake(float amplitude, float durationSec = 0.38f) {
        shakeIntensity = amplitude;
        shakeTimer     = durationSec;
    }

    void triggerSlowMotion(float scale, float durationSec = 0.85f) {
        slowMoScale = constrain(scale, 0.15f, 1.0f);
        slowMoTimer = durationSec;
    }

    float update(float rawDt) {
        zoom += (targetZoom - zoom) * min(1.0f, rawDt * 7.5f);

        if (shakeTimer > 0.0f) {
            shakeTimer -= rawDt;
            float decay = max(0.0f, shakeTimer * 2.5f);
            shakeOffsetX = ((float)random(-100, 101) * 0.01f) * shakeIntensity * min(1.0f, decay);
            shakeOffsetY = ((float)random(-100, 101) * 0.01f) * shakeIntensity * min(1.0f, decay);
        } else {
            shakeOffsetX = 0.0f;
            shakeOffsetY = 0.0f;
        }

        if (slowMoTimer > 0.0f) {
            slowMoTimer -= rawDt;
            if (slowMoTimer <= 0.0f) slowMoScale = 1.0f;
        }
        return rawDt * slowMoScale;
    }

    inline int16_t worldToScreenX(float wx) const {
        return (int16_t)(anchorX * zoom + (wx - anchorX) * zoom + shakeOffsetX);
    }

    inline int16_t worldToScreenY(float wy) const {
        return (int16_t)(anchorY - (anchorY - wy) * zoom + shakeOffsetY);
    }

    inline int16_t scaleDim(float v) const {
        return max((int16_t)1, (int16_t)(v * zoom));
    }
};

// ============================================================================
// 13. BLOOD SPLATTER, ARTERIAL DROPLETS & SURFACE DECAL SYSTEM
// ============================================================================
struct BloodDroplet {
    bool active;
    Vec2 pos;
    Vec2 vel;
    float radius;
    uint16_t color;
};

struct GroundBloodDecal {
    bool active;
    float x;
    int16_t width;
    uint8_t alpha;
};

class BloodAndDecalSystem2D
{
public:
    static constexpr int MAX_DROPLETS = 56;
    static constexpr int MAX_DECALS   = 24;

    BloodDroplet droplets[MAX_DROPLETS];
    GroundBloodDecal decals[MAX_DECALS];
    int nextDecalIdx = 0;

    void clear() {
        for (int i = 0; i < MAX_DROPLETS; i++) droplets[i].active = false;
        for (int i = 0; i < MAX_DECALS; i++)   decals[i].active = false;
    }

    void emitBloodSplatter(float x, float y, int count = 36, float baseVx = -180.0f, float baseVy = -340.0f) {
        const uint16_t bloodPalette[4] = {
            Colors565::Crimson, Colors565::DarkRed, Colors565::Scarlet, Colors565::Maroon
        };
        int spawned = 0;
        for (int i = 0; i < MAX_DROPLETS && spawned < count; i++) {
            if (!droplets[i].active) {
                droplets[i].active = true;
                droplets[i].pos    = Vec2(x + random(-12, 13), y + random(-16, 17));
                droplets[i].vel    = Vec2(baseVx + random(-240, 260), baseVy + random(-260, 140));
                droplets[i].radius = (float)random(2, 6);
                droplets[i].color  = bloodPalette[i & 3];
                spawned++;
            }
        }
    }

    void update(float dt, float groundY, float scrollSpeedX = 0.0f) {
        for (int i = 0; i < MAX_DECALS; i++) {
            if (decals[i].active) {
                decals[i].x -= scrollSpeedX * dt;
                if (decals[i].x < -60.0f) decals[i].active = false;
            }
        }

        for (int i = 0; i < MAX_DROPLETS; i++) {
            if (!droplets[i].active) continue;
            droplets[i].vel.y += 1450.0f * dt;
            droplets[i].pos   += droplets[i].vel * dt;

            if (droplets[i].pos.y >= groundY) {
                droplets[i].active = false;
                decals[nextDecalIdx].active = true;
                decals[nextDecalIdx].x      = droplets[i].pos.x;
                decals[nextDecalIdx].width  = (int16_t)random(10, 26);
                decals[nextDecalIdx].alpha  = 210;
                nextDecalIdx = (nextDecalIdx + 1) % MAX_DECALS;
            }
        }
    }

    void draw(RGBGameEngine &eng, int16_t groundY, const Camera2D *cam = nullptr) {
        for (int i = 0; i < MAX_DECALS; i++) {
            if (!decals[i].active) continue;
            int16_t dx = cam ? cam->worldToScreenX(decals[i].x) : (int16_t)decals[i].x;
            int16_t dw = cam ? cam->scaleDim(decals[i].width) : decals[i].width;
            eng.fillRectAlpha(dx - dw / 2, groundY - 2, dw, 5, Colors565::Crimson, decals[i].alpha);
        }
        for (int i = 0; i < MAX_DROPLETS; i++) {
            if (!droplets[i].active) continue;
            int16_t px = cam ? cam->worldToScreenX(droplets[i].pos.x) : (int16_t)droplets[i].pos.x;
            int16_t py = cam ? cam->worldToScreenY(droplets[i].pos.y) : (int16_t)droplets[i].pos.y;
            int16_t tx = cam ? cam->worldToScreenX(droplets[i].pos.x - droplets[i].vel.x * 0.025f) : (int16_t)(droplets[i].pos.x - droplets[i].vel.x * 0.025f);
            int16_t ty = cam ? cam->worldToScreenY(droplets[i].pos.y - droplets[i].vel.y * 0.025f) : (int16_t)(droplets[i].pos.y - droplets[i].vel.y * 0.025f);
            int16_t r  = cam ? cam->scaleDim(droplets[i].radius) : (int16_t)droplets[i].radius;
            eng.drawThickLine(px, py, tx, ty, r, droplets[i].color, true);
        }
    }
};

// ============================================================================
// 14. ARTICULATED 2D SKELETAL RIG & RAGDOLL FALL-DOWN SOLVER
// ============================================================================
class SkeletalRig2D
{
public:
    bool ragdollActive = false;
    Vec2 pelvisPos, pelvisVel;
    float torsoAngle = 0.0f;
    float angularVel = 0.0f;

    void triggerRagdollFall(float startX, float startY, float impactVx = -160.0f, float impactVy = -320.0f) {
        ragdollActive = true;
        pelvisPos  = Vec2(startX, startY);
        pelvisVel  = Vec2(impactVx, impactVy);
        torsoAngle = 0.0f;
        angularVel = -5.2f;
    }

    void update(float dt, float groundY) {
        if (!ragdollActive) return;
        pelvisVel.y += 1550.0f * dt;
        pelvisPos   += pelvisVel * dt;
        torsoAngle  += angularVel * dt;

        if (pelvisPos.y >= groundY - 14.0f) {
            pelvisPos.y = groundY - 14.0f;
            pelvisVel.y *= -0.32f;
            pelvisVel.x *= 0.72f;
            angularVel  *= 0.55f;
            torsoAngle = constrain(torsoAngle, -1.52f, 1.52f);
        }
    }
};

// ============================================================================
// 15. RADIAL & VORTEX FORCE FIELD 2D
// ============================================================================
enum class ForceFieldType : uint8_t {
    VORTEX_PULL      = 0,
    GRAVITY_INVERT   = 1,
    REPULSE_WAVE     = 2,
    MAGNETIC_DIPOLE  = 3,
    TIME_SLOW_AURA   = 4
};

class ForceField2D
{
public:
    Vec2 center;
    float radius = 140.0f;
    float strength = 480.0f;
    ForceFieldType type = ForceFieldType::VORTEX_PULL;

    Vec2 computeAcceleration(Vec2 targetPos, Vec2 targetVel) const {
        Vec2 d = center - targetPos;
        float d2 = d.lengthSq();
        if (d2 > radius * radius || d2 < 4.0f) return Vec2(0, 0);
        float dist = sqrtf(d2);
        Vec2 dir = d * (1.0f / dist);
        float falloff = 1.0f - (dist / radius);

        switch (type) {
            case ForceFieldType::VORTEX_PULL:
                return dir * (strength * falloff) + Vec2(-dir.y, dir.x) * (strength * 0.65f * falloff);
            case ForceFieldType::GRAVITY_INVERT:
                return Vec2(0.0f, -strength * 2.2f);
            case ForceFieldType::REPULSE_WAVE:
                return dir * (-strength * 1.5f * falloff);
            case ForceFieldType::MAGNETIC_DIPOLE:
                return dir * (strength * falloff * falloff);
            case ForceFieldType::TIME_SLOW_AURA:
            default:
                return targetVel * (-3.5f * falloff);
        }
    }
};

// ============================================================================
// 16. RLE COMPRESSED SPRITE & MULTI-STATE FLIPBOOK ANIMATOR
// ============================================================================
class SpriteAnimator2D
{
public:
    int currentFrame = 0;
    int frameCount   = 16;
    float frameTimer = 0.0f;
    float fps        = 30.0f;
    bool looping     = true;

    void update(float dt) {
        frameTimer += dt;
        float interval = 1.0f / max(1.0f, fps);
        while (frameTimer >= interval) {
            frameTimer -= interval;
            if (currentFrame + 1 < frameCount) {
                currentFrame++;
            } else if (looping) {
                currentFrame = 0;
            }
        }
    }
};

#endif
