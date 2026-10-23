#ifndef RGBGE_POINT_H
#define RGBGE_POINT_H

#include <stdint.h>
#include <math.h>

// ============================================================================
// Integer 2D Point (Screen Coordinates)
// ============================================================================
struct Point
{
    int16_t x;
    int16_t y;

    constexpr Point() : x(0), y(0) {}
    constexpr Point(int16_t px, int16_t py) : x(px), y(py) {}
};

// ============================================================================
// Floating-Point 2D Vector (Physics, Kinematics, Lighting & Splines)
// ============================================================================
struct Vec2
{
    float x;
    float y;

    constexpr Vec2() : x(0.0f), y(0.0f) {}
    constexpr Vec2(float px, float py) : x(px), y(py) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const       { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const       { return (s != 0.0f) ? Vec2(x / s, y / s) : Vec2(0.0f, 0.0f); }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s)       { x *= s;   y *= s;   return *this; }

    float dot(const Vec2& o) const   { return x * o.x + y * o.y; }
    float cross(const Vec2& o) const { return x * o.y - y * o.x; }
    float lengthSq() const           { return x * x + y * y; }
    float length() const             { return sqrtf(x * x + y * y); }

    Vec2 normalized() const {
        float len = length();
        return (len > 0.00001f) ? Vec2(x / len, y / len) : Vec2(0.0f, 0.0f);
    }

    Vec2 perp() const { return Vec2(-y, x); }

    Vec2 rotated(float angleRad) const {
        float c = cosf(angleRad);
        float s = sinf(angleRad);
        return Vec2(x * c - y * s, x * s + y * c);
    }

    static Vec2 lerp(const Vec2& a, const Vec2& b, float t) {
        return Vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }

    static float dist(const Vec2& a, const Vec2& b) {
        return (a - b).length();
    }

    Point toPoint() const {
        return Point((int16_t)(x + 0.5f), (int16_t)(y + 0.5f));
    }
};

// ============================================================================
// Floating-Point 3D Vector (2.5D Wireframe & Flat-Shaded Projections)
// ============================================================================
struct Vec3
{
    float x, y, z;

    constexpr Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    constexpr Vec3(float px, float py, float pz) : x(px), y(py), z(pz) {}

    Vec3 rotateX(float a) const {
        float c = cosf(a), s = sinf(a);
        return Vec3(x, y * c - z * s, y * s + z * c);
    }
    Vec3 rotateY(float a) const {
        float c = cosf(a), s = sinf(a);
        return Vec3(x * c + z * s, y, -x * s + z * c);
    }
    Vec3 rotateZ(float a) const {
        float c = cosf(a), s = sinf(a);
        return Vec3(x * c - y * s, x * s + y * c, z);
    }

    Point project(int16_t centerX, int16_t centerY, float fov = 950.0f, float viewerDist = 950.0f) const {
        float zDist = z + viewerDist;
        if (zDist < 50.0f) zDist = 50.0f;
        float scale = fov / zDist;
        return Point((int16_t)roundf(centerX + x * scale), (int16_t)roundf(centerY + y * scale));
    }
};

// ============================================================================
// 2D Axis-Aligned Bounding Box (AABB)
// ============================================================================
struct Rect2D
{
    float x, y, w, h;

    constexpr Rect2D() : x(0), y(0), w(0), h(0) {}
    constexpr Rect2D(float px, float py, float pw, float ph) : x(px), y(py), w(pw), h(ph) {}

    bool contains(float px, float py) const {
        return (px >= x && px <= x + w && py >= y && py <= y + h);
    }

    bool intersects(const Rect2D& o) const {
        return !(x + w < o.x || o.x + o.w < x || y + h < o.y || o.y + o.h < y);
    }
};

#endif
