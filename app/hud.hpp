#pragma once

#include <SFML/Graphics/Rect.hpp>

#include "asteroids/game.hpp"
#include "renderer.hpp"

namespace app {

/// Live-tunable settings exposed through the debug panel.
struct DebugState {
    bool visible{false};
    bool paused{false};
    float time_scale{1.0F};
    int ticks_last_frame{0};
    RenderOptions render{};
};

/// Score, wave and on-screen messages. `viewport` is the game area in window
/// pixels (it excludes letterbox bars).
void draw_hud(const asteroids::Game& game, sf::IntRect viewport);

/// Developer overlay: frame timing, entity counts, broad-phase stats and cheats.
void draw_debug_panel(asteroids::Game& game, DebugState& state, sf::IntRect viewport);

}  // namespace app
