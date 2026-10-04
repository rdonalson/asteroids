#pragma once

#include <cmath>

#include "asteroids/config.hpp"

namespace asteroids {

struct Vec2 {
    float x{};
    float y{};

    constexpr Vec2& operator+=(Vec2 rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    constexpr Vec2& operator-=(Vec2 rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    constexpr Vec2& operator*=(float s) noexcept {
        x *= s;
        y *= s;
        return *this;
    }

    friend constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { return a += b; }
    friend constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return a -= b; }
    friend constexpr Vec2 operator*(Vec2 v, float s) noexcept { return v *= s; }
    friend constexpr Vec2 operator*(float s, Vec2 v) noexcept { return v *= s; }
    friend constexpr bool operator==(Vec2 a, Vec2 b) noexcept = default;
};

[[nodiscard]] constexpr float dot(Vec2 a, Vec2 b) noexcept { return a.x * b.x + a.y * b.y; }
[[nodiscard]] constexpr float length_sq(Vec2 v) noexcept { return dot(v, v); }
[[nodiscard]] inline float length(Vec2 v) noexcept { return std::sqrt(length_sq(v)); }

/// Unit vector for a heading. Angle 0 points up the screen (y grows downward).
[[nodiscard]] inline Vec2 from_angle(float radians) noexcept {
    return {std::sin(radians), -std::cos(radians)};
}

/// Rotate a vector by an angle (screen coordinates, clockwise for positive angles).
[[nodiscard]] inline Vec2 rotate(Vec2 v, float radians) noexcept {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}

/// Wrap a coordinate into [0, max).
[[nodiscard]] inline float wrap(float value, float max) noexcept {
    float r = std::fmod(value, max);
    if (r < 0.0F) {
        r += max;
    }
    return r >= max ? 0.0F : r;
}

/// The world is a torus: leaving one edge re-enters from the opposite edge.
[[nodiscard]] inline Vec2 wrap_position(Vec2 p) noexcept {
    return {wrap(p.x, config::kWorldWidth), wrap(p.y, config::kWorldHeight)};
}

/// Shortest vector from a to b on the torus.
[[nodiscard]] constexpr Vec2 wrapped_delta(Vec2 a, Vec2 b) noexcept {
    constexpr float kHalfW = config::kWorldWidth / 2.0F;
    constexpr float kHalfH = config::kWorldHeight / 2.0F;
    Vec2 d = b - a;
    if (d.x > kHalfW) {
        d.x -= config::kWorldWidth;
    } else if (d.x < -kHalfW) {
        d.x += config::kWorldWidth;
    }
    if (d.y > kHalfH) {
        d.y -= config::kWorldHeight;
    } else if (d.y < -kHalfH) {
        d.y += config::kWorldHeight;
    }
    return d;
}

/// True if two circles overlap, accounting for screen wrap-around.
[[nodiscard]] constexpr bool circles_overlap(Vec2 a, float ra, Vec2 b, float rb) noexcept {
    const float r = ra + rb;
    return length_sq(wrapped_delta(a, b)) < r * r;
}

}  // namespace asteroids
