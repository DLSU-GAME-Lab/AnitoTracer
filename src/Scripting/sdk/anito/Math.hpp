#pragma once

// Math category: value types and helpers that run entirely inside the script module (no host calls).
// Conventions match the engine: left-handed, +Z forward, quaternions rotate with q * v.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

namespace anito {

struct Vec3 {
    float x, y, z;

    float lengthSq() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(lengthSq()); }
    float dot(Vec3 o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(Vec3 o) const { return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x}; }
    Vec3 normalized() const {
        const float l = length();
        return l > 1e-8f ? Vec3{x / l, y / l, z / l} : Vec3{0.0f, 0.0f, 0.0f};
    }
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(float s, Vec3 a) { return a * s; }
inline Vec3 operator/(Vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
inline Vec3& operator+=(Vec3& a, Vec3 b) { return a = a + b; }
inline Vec3& operator-=(Vec3& a, Vec3 b) { return a = a - b; }
inline Vec3& operator*=(Vec3& a, float s) { return a = a * s; }
inline Vec3& operator/=(Vec3& a, float s) { return a = a / s; }
inline bool operator==(Vec3 a, Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
inline bool operator!=(Vec3 a, Vec3 b) { return !(a == b); }

struct Quat {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

    float dot(Quat o) const { return x * o.x + y * o.y + z * o.z + w * o.w; }
    Quat normalized() const {
        const float l = std::sqrt(dot(*this));
        return l > 1e-8f ? Quat{x / l, y / l, z / l, w / l} : Quat{};
    }
    Quat inverse() const {
        const float d = dot(*this);
        return d > 1e-8f ? Quat{-x / d, -y / d, -z / d, w / d} : Quat{};
    }
};

// Hamilton product, same composition order as glm: (a * b) applies b first, then a.
inline Quat operator*(Quat a, Quat b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
            a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

inline Vec3 operator*(Quat q, Vec3 v) {
    const Vec3 qv{q.x, q.y, q.z};
    const Vec3 uv = qv.cross(v);
    const Vec3 uuv = qv.cross(uv);
    return v + (uv * q.w + uuv) * 2.0f;
}

struct Mathf {
    static constexpr float pi() { return 3.14159265358979323846f; }
    static constexpr float deg2rad() { return pi() / 180.0f; }
    static constexpr float rad2deg() { return 180.0f / pi(); }
    static constexpr float epsilon() { return 1e-6f; }

    static float clamp(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }
    static float clamp01(float v) { return clamp(v, 0.0f, 1.0f); }
    static float lerp(float a, float b, float t) { return a + (b - a) * t; }
    static float sign(float v) { return v < 0.0f ? -1.0f : 1.0f; }
    static float sin(float v) { return std::sin(v); }
    static float cos(float v) { return std::cos(v); }
    static float tan(float v) { return std::tan(v); }
    static float asin(float v) { return std::asin(v); }
    static float acos(float v) { return std::acos(v); }
    static float atan(float v) { return std::atan(v); }
    static float atan2(float y, float x) { return std::atan2(y, x); }
    static float sqrt(float v) { return std::sqrt(v); }
    static float cbrt(float v) { return std::cbrt(v); }
    static float pow(float v, float e) { return std::pow(v, e); }
    static float exp(float v) { return std::exp(v); }
    static float abs(float v) { return std::fabs(v); }
    static float floor(float v) { return std::floor(v); }
    static float ceil(float v) { return std::ceil(v); }
    static float round(float v) { return std::round(v); }
    static float min(float a, float b) { return std::fmin(a, b); }
    static float max(float a, float b) { return std::fmax(a, b); }
    static float moveTowards(float current, float target, float maxDelta) {
        return std::fabs(target - current) <= maxDelta ? target : current + (target < current ? -maxDelta : maxDelta);
    }
};

struct Vector3 {
    static Vec3 zero() { return {0.0f, 0.0f, 0.0f}; }
    static Vec3 one() { return {1.0f, 1.0f, 1.0f}; }
    static Vec3 up() { return {0.0f, 1.0f, 0.0f}; }
    static Vec3 forward() { return {0.0f, 0.0f, 1.0f}; }
    static Vec3 right() { return {1.0f, 0.0f, 0.0f}; }

    static float dot(Vec3 a, Vec3 b) { return a.dot(b); }
    static Vec3 cross(Vec3 a, Vec3 b) { return a.cross(b); }
    static float length(Vec3 v) { return v.length(); }
    static float distance(Vec3 a, Vec3 b) { return (a - b).length(); }
    static Vec3 normalize(Vec3 v) { return v.normalized(); }
    static Vec3 lerp(Vec3 a, Vec3 b, float t) { return a + (b - a) * t; }
    static Vec3 scale(Vec3 a, Vec3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
};

struct Quaternion {
    static Quat identity() { return Quat{}; }

    static float dot(Quat a, Quat b) { return a.dot(b); }

    static Quat angleAxis(float degrees, Vec3 axis) {
        const float lengthSq = axis.lengthSq();
        if (lengthSq < 1e-8f) return Quat{};
        const Vec3 n = axis / std::sqrt(lengthSq);
        const float half = degrees * Mathf::deg2rad() * 0.5f;
        const float s = std::sin(half);
        return {n.x * s, n.y * s, n.z * s, std::cos(half)};
    }

    // Euler angles in degrees, applied in the engine's pitch(x)/yaw(y)/roll(z) order.
    static Quat euler(Vec3 degrees) {
        const Vec3 h = degrees * (Mathf::deg2rad() * 0.5f);
        const float cx = std::cos(h.x), cy = std::cos(h.y), cz = std::cos(h.z);
        const float sx = std::sin(h.x), sy = std::sin(h.y), sz = std::sin(h.z);
        return {sx * cy * cz - cx * sy * sz, cx * sy * cz + sx * cy * sz, cx * cy * sz - sx * sy * cz,
                cx * cy * cz + sx * sy * sz};
    }

    // Rotation whose +Z faces `forward` with +Y as close to `up` as possible.
    static Quat lookRotation(Vec3 forward, Vec3 up) {
        const Vec3 f = forward.normalized();
        if (f.lengthSq() < 0.5f) return Quat{};
        Vec3 right = up.cross(f);
        right = right / std::sqrt(std::max(0.00001f, right.dot(right)));
        const Vec3 u = f.cross(right);

        // Columns right/u/f form the rotation matrix; extract the quaternion from the largest diagonal term.
        const float m00 = right.x, m11 = u.y, m22 = f.z;
        const float fourW = m00 + m11 + m22;
        const float fourX = m00 - m11 - m22;
        const float fourY = m11 - m00 - m22;
        const float fourZ = m22 - m00 - m11;

        int biggest = 0;
        float fourBiggest = fourW;
        if (fourX > fourBiggest) { fourBiggest = fourX; biggest = 1; }
        if (fourY > fourBiggest) { fourBiggest = fourY; biggest = 2; }
        if (fourZ > fourBiggest) { fourBiggest = fourZ; biggest = 3; }

        const float value = std::sqrt(fourBiggest + 1.0f) * 0.5f;
        const float mult = 0.25f / value;
        switch (biggest) {
        case 0: return {(u.z - f.y) * mult, (f.x - right.z) * mult, (right.y - u.x) * mult, value};
        case 1: return {value, (right.y + u.x) * mult, (f.x + right.z) * mult, (u.z - f.y) * mult};
        case 2: return {(right.y + u.x) * mult, value, (u.z + f.y) * mult, (f.x - right.z) * mult};
        default: return {(f.x + right.z) * mult, (u.z + f.y) * mult, value, (right.y - u.x) * mult};
        }
    }

    // Shortest-path spherical interpolation.
    static Quat slerp(Quat a, Quat b, float t) {
        Quat z = b;
        float cosTheta = a.dot(b);
        if (cosTheta < 0.0f) {
            z = {-b.x, -b.y, -b.z, -b.w};
            cosTheta = -cosTheta;
        }
        if (cosTheta > 1.0f - 1e-6f) {
            return {Mathf::lerp(a.x, z.x, t), Mathf::lerp(a.y, z.y, t), Mathf::lerp(a.z, z.z, t), Mathf::lerp(a.w, z.w, t)};
        }
        const float angle = std::acos(cosTheta);
        const float sa = std::sin((1.0f - t) * angle), sb = std::sin(t * angle), inv = 1.0f / std::sin(angle);
        return {(a.x * sa + z.x * sb) * inv, (a.y * sa + z.y * sb) * inv, (a.z * sa + z.z * sb) * inv,
                (a.w * sa + z.w * sb) * inv};
    }
};

namespace detail {
inline std::mt19937& Rng() {
    static thread_local std::mt19937 rng{std::random_device{}()};
    return rng;
}
} // namespace detail

struct Random {
    static float value() { return std::uniform_real_distribution<float>(0.0f, 1.0f)(detail::Rng()); }
    static float range(float lo, float hi) { return lo + (hi - lo) * value(); }

    // Uniform direction on the unit sphere.
    static Vec3 onUnitSphere() {
        const float z = value() * 2.0f - 1.0f;
        const float angle = value() * 2.0f * Mathf::pi();
        const float ring = std::sqrt(std::max(0.0f, 1.0f - z * z));
        return {ring * std::cos(angle), ring * std::sin(angle), z};
    }

    // Uniform density inside the unit sphere (cube-root radius).
    static Vec3 insideUnitSphere() { return onUnitSphere() * std::cbrt(value()); }

    static Quat rotation() {
        const float x = value() - 0.5f, y = value() - 0.5f, z = value() - 0.5f;
        return Quaternion::angleAxis(value() * 360.0f, Vec3{x + 1e-4f, y + 1e-4f, z + 1e-4f}.normalized());
    }
};

} // namespace anito
