#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <array>
#include <cstdint>
#include <span>

#include "asteroids/game.hpp"

namespace app {

struct RenderOptions {
    bool show_grid{false};
};

/// Draws the game as glowing vector lines, like the 1979 arcade original.
///
/// Every line is a thin quad appended to one vertex array, so a whole frame is a
/// single draw call. The array is cleared (not freed) each frame, so after the
/// first few frames rendering does not allocate either.
class Renderer {
public:
    Renderer();

    void draw(sf::RenderTarget& target, const asteroids::Game& game, const RenderOptions& options);

private:
    struct Star {
        asteroids::Vec2 pos;
        std::uint8_t brightness;
    };

    void add_line(asteroids::Vec2 a, asteroids::Vec2 b, sf::Color color, float width);
    void add_polyline(std::span<const asteroids::Vec2> local, bool closed, asteroids::Vec2 center,
                      float angle, float scale, float bounds, sf::Color color);

    void draw_starfield();
    void draw_grid();
    void draw_asteroid(const asteroids::Asteroid& asteroid);
    void draw_ship(const asteroids::Ship& ship, std::uint64_t tick);
    void draw_bullets(std::span<const asteroids::Bullet> bullets);
    void draw_particles(std::span<const asteroids::Particle> particles);
    void draw_lives(int lives);

    sf::VertexArray triangles_{sf::PrimitiveType::Triangles};
    std::array<Star, 140> stars_{};
};

}  // namespace app
