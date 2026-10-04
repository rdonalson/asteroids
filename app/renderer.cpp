#include "renderer.hpp"

#include <SFML/Graphics/Vertex.hpp>
#include <algorithm>
#include <numbers>

#include "asteroids/rng.hpp"

namespace app {

using asteroids::Vec2;
namespace config = asteroids::config;

namespace {

constexpr float kLineWidth = 1.8F;
constexpr std::size_t kAsteroidVertices = 11;

constexpr sf::Color kForeground{225, 238, 255};
constexpr sf::Color kBulletColor{255, 230, 140};
constexpr sf::Color kFlameColor{255, 150, 60};
constexpr sf::Color kGridColor{80, 140, 255, 45};

// Ship outline in local space, nose pointing up (angle 0).
constexpr std::array<Vec2, 4> kShipShape{
    {{0.0F, -16.0F}, {10.0F, 12.0F}, {0.0F, 7.0F}, {-10.0F, 12.0F}}};
constexpr std::array<Vec2, 3> kFlameShape{{{-5.0F, 9.0F}, {0.0F, 22.0F}, {5.0F, 9.0F}}};

/// Small integer hash: turns (seed, vertex) into a stable pseudo-random value.
constexpr std::uint32_t mix32(std::uint32_t x) noexcept {
    x ^= x >> 16U;
    x *= 0x7FEB352DU;
    x ^= x >> 15U;
    x *= 0x846CA68BU;
    x ^= x >> 16U;
    return x;
}

sf::Vector2f to_sf(Vec2 v) { return {v.x, v.y}; }

}  // namespace

Renderer::Renderer() {
    asteroids::Rng rng(7U);  // same sky every launch
    for (Star& s : stars_) {
        s.pos = {rng.range(0.0F, config::kWorldWidth), rng.range(0.0F, config::kWorldHeight)};
        s.brightness = static_cast<std::uint8_t>(rng.range(40.0F, 140.0F));
    }
}

void Renderer::draw(sf::RenderTarget& target, const asteroids::Game& game,
                    const RenderOptions& options) {
    triangles_.clear();

    draw_starfield();
    if (options.show_grid) {
        draw_grid();
    }
    for (const auto& a : game.asteroids()) {
        draw_asteroid(a);
    }
    draw_particles(game.particles());
    draw_bullets(game.bullets());
    if (game.ship().alive && game.phase() == asteroids::Phase::kPlaying) {
        draw_ship(game.ship(), game.tick());
    }
    draw_lives(game.lives());

    target.draw(triangles_);
}

void Renderer::add_line(Vec2 a, Vec2 b, sf::Color color, float width) {
    const Vec2 d = b - a;
    const float len = asteroids::length(d);
    if (len < 1e-4F) {
        return;
    }
    const Vec2 n = Vec2{-d.y, d.x} * (0.5F * width / len);
    for (const Vec2 p : {a + n, b + n, b - n, a + n, b - n, a - n}) {
        triangles_.append(sf::Vertex{to_sf(p), color});
    }
}

void Renderer::add_polyline(std::span<const Vec2> local, bool closed, Vec2 center, float angle,
                            float scale, float bounds, sf::Color color) {
    // Objects overlapping a screen edge are also drawn on the opposite side, so
    // they slide smoothly across the wrap instead of popping.
    std::array<float, 2> xs{0.0F, 0.0F};
    std::array<float, 2> ys{0.0F, 0.0F};
    std::size_t nx = 1;
    std::size_t ny = 1;
    if (center.x - bounds < 0.0F) {
        xs[nx++] = config::kWorldWidth;
    } else if (center.x + bounds > config::kWorldWidth) {
        xs[nx++] = -config::kWorldWidth;
    }
    if (center.y - bounds < 0.0F) {
        ys[ny++] = config::kWorldHeight;
    } else if (center.y + bounds > config::kWorldHeight) {
        ys[ny++] = -config::kWorldHeight;
    }

    const std::size_t segments = closed ? local.size() : local.size() - 1;
    for (std::size_t iy = 0; iy < ny; ++iy) {
        for (std::size_t ix = 0; ix < nx; ++ix) {
            const Vec2 origin = center + Vec2{xs[ix], ys[iy]};
            for (std::size_t i = 0; i < segments; ++i) {
                const Vec2 a = origin + asteroids::rotate(local[i] * scale, angle);
                const Vec2 b =
                    origin + asteroids::rotate(local[(i + 1) % local.size()] * scale, angle);
                add_line(a, b, color, kLineWidth);
            }
        }
    }
}

void Renderer::draw_starfield() {
    for (const Star& s : stars_) {
        const sf::Color c{s.brightness, s.brightness,
                          static_cast<std::uint8_t>(s.brightness + 30U)};
        add_line(s.pos, s.pos + Vec2{1.6F, 0.0F}, c, 1.6F);
    }
}

void Renderer::draw_grid() {
    constexpr float kCellW = config::kWorldWidth / static_cast<float>(config::kGridCellsX);
    constexpr float kCellH = config::kWorldHeight / static_cast<float>(config::kGridCellsY);
    for (std::size_t x = 1; x < config::kGridCellsX; ++x) {
        const float px = static_cast<float>(x) * kCellW;
        add_line({px, 0.0F}, {px, config::kWorldHeight}, kGridColor, 1.0F);
    }
    for (std::size_t y = 1; y < config::kGridCellsY; ++y) {
        const float py = static_cast<float>(y) * kCellH;
        add_line({0.0F, py}, {config::kWorldWidth, py}, kGridColor, 1.0F);
    }
}

void Renderer::draw_asteroid(const asteroids::Asteroid& asteroid) {
    // A jagged outline derived from the rock's seed, so each rock keeps its own shape.
    std::array<Vec2, kAsteroidVertices> outline{};
    constexpr float kStep =
        2.0F * std::numbers::pi_v<float> / static_cast<float>(kAsteroidVertices);
    for (std::size_t i = 0; i < kAsteroidVertices; ++i) {
        const std::uint32_t h =
            mix32(asteroid.shape_seed + static_cast<std::uint32_t>(i) * 0x9E3779B9U);
        const float jag = 0.72F + 0.28F * static_cast<float>(h & 0xFFFFU) / 65535.0F;
        outline[i] = asteroids::from_angle(static_cast<float>(i) * kStep) * jag;
    }
    const float r = asteroids::radius_of(asteroid.size);
    add_polyline(outline, true, asteroid.pos, asteroid.angle, r, r, kForeground);
}

void Renderer::draw_ship(const asteroids::Ship& ship, std::uint64_t tick) {
    if (ship.invulnerable > 0.0F && (tick / 6) % 2 == 0) {
        return;  // blink while spawn-protected
    }
    add_polyline(kShipShape, true, ship.pos, ship.angle, 1.0F, 22.0F, kForeground);
    if (ship.thrusting && (tick / 2) % 2 == 0) {
        add_polyline(kFlameShape, false, ship.pos, ship.angle, 1.0F, 22.0F, kFlameColor);
    }
}

void Renderer::draw_bullets(std::span<const asteroids::Bullet> bullets) {
    for (const auto& b : bullets) {
        const float speed = asteroids::length(b.vel);
        const Vec2 tail = speed > 0.0F ? b.pos - b.vel * (6.0F / speed) : b.pos;
        add_line(tail, b.pos, kBulletColor, 2.4F);
    }
}

void Renderer::draw_particles(std::span<const asteroids::Particle> particles) {
    for (const auto& p : particles) {
        const float t = std::clamp(p.life / config::kParticleLife, 0.0F, 1.0F);
        const sf::Color c{kForeground.r, kForeground.g, kForeground.b,
                          static_cast<std::uint8_t>(255.0F * t)};
        add_line(p.pos - p.vel * 0.02F, p.pos, c, 1.4F);
    }
}

void Renderer::draw_lives(int lives) {
    const int shown = std::min(lives, 10);
    for (int i = 0; i < shown; ++i) {
        const Vec2 pos{28.0F + static_cast<float>(i) * 22.0F, 92.0F};
        add_polyline(kShipShape, true, pos, 0.0F, 0.7F, 0.0F, kForeground);
    }
}

}  // namespace app
