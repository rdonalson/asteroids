#include <imgui-SFML.h>
#include <imgui.h>

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <random>

#include "asteroids/game.hpp"
#include "hud.hpp"
#include "renderer.hpp"

namespace {

namespace config = asteroids::config;

/// Scale the fixed-size world to fit the window, adding black bars as needed.
void apply_letterbox(sf::RenderWindow& window, sf::Vector2u window_size) {
    const float window_ratio =
        static_cast<float>(window_size.x) / static_cast<float>(window_size.y);
    const float world_ratio = config::kWorldWidth / config::kWorldHeight;

    sf::FloatRect viewport({0.0F, 0.0F}, {1.0F, 1.0F});
    if (window_ratio > world_ratio) {
        viewport.size.x = world_ratio / window_ratio;
        viewport.position.x = (1.0F - viewport.size.x) / 2.0F;
    } else {
        viewport.size.y = window_ratio / world_ratio;
        viewport.position.y = (1.0F - viewport.size.y) / 2.0F;
    }

    sf::View view(sf::FloatRect({0.0F, 0.0F}, {config::kWorldWidth, config::kWorldHeight}));
    view.setViewport(viewport);
    window.setView(view);
}

bool key_down(sf::Keyboard::Key a, sf::Keyboard::Key b) {
    return sf::Keyboard::isKeyPressed(a) || sf::Keyboard::isKeyPressed(b);
}

asteroids::Input read_input(const sf::RenderWindow& window) {
    using Key = sf::Keyboard::Key;
    if (!window.hasFocus() || ImGui::GetIO().WantCaptureKeyboard) {
        return {};
    }
    return {.rotate_left = key_down(Key::Left, Key::A),
            .rotate_right = key_down(Key::Right, Key::D),
            .thrust = key_down(Key::Up, Key::W),
            .fire = sf::Keyboard::isKeyPressed(Key::Space)};
}

}  // namespace

int main() {
    sf::RenderWindow window(sf::VideoMode({1280U, 720U}), "Asteroids");
    window.setVerticalSyncEnabled(true);
    apply_letterbox(window, window.getSize());

    if (!ImGui::SFML::Init(window)) {
        return EXIT_FAILURE;
    }
    ImGui::GetIO().IniFilename = nullptr;  // don't litter the folder with imgui.ini

    // Random seed per launch; it is shown in the debug panel so a run can be reproduced.
    const std::uint64_t seed =
        (static_cast<std::uint64_t>(std::random_device{}()) << 32U) | std::random_device{}();
    auto game = std::make_unique<asteroids::Game>(seed);
    app::Renderer renderer;
    app::DebugState debug;

    sf::Clock frame_clock;
    float accumulator = 0.0F;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                apply_letterbox(window, resized->size);
            } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                switch (key->code) {
                    case sf::Keyboard::Key::Escape:
                        window.close();
                        break;
                    case sf::Keyboard::Key::F1:
                        debug.visible = !debug.visible;
                        break;
                    case sf::Keyboard::Key::P:
                        debug.paused = !debug.paused;
                        break;
                    case sf::Keyboard::Key::Enter:
                        if (game->phase() == asteroids::Phase::kGameOver) {
                            game->restart();
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        if (!window.isOpen()) {
            break;
        }

        const sf::Time frame_time = frame_clock.restart();
        ImGui::SFML::Update(window, frame_time);

        // Fixed timestep: the simulation always advances in exact 1/60 s steps,
        // however fast or slow frames are drawn. The clamp stops a long stall
        // (e.g. dragging the window) from causing a burst of catch-up steps.
        if (!debug.paused) {
            accumulator += std::min(frame_time.asSeconds(), 0.25F) * debug.time_scale;
        }
        const asteroids::Input input = read_input(window);
        debug.ticks_last_frame = 0;
        while (accumulator >= config::kDt) {
            game->step(input);
            accumulator -= config::kDt;
            ++debug.ticks_last_frame;
        }

        const sf::IntRect viewport = window.getViewport(window.getView());
        app::draw_hud(*game, viewport);
        app::draw_debug_panel(*game, debug, viewport);

        window.clear(sf::Color::Black);
        renderer.draw(window, *game, debug.render);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return EXIT_SUCCESS;
}
