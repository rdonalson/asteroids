#include "hud.hpp"

#include <imgui.h>

#include <cstdint>

namespace app {

namespace {

constexpr ImGuiWindowFlags kOverlayFlags =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

constexpr std::uint64_t kShowControlsTicks = 60ULL * 5ULL;  // 5 seconds

ImVec2 origin(sf::IntRect vp) {
    return {static_cast<float>(vp.position.x), static_cast<float>(vp.position.y)};
}

ImVec2 size(sf::IntRect vp) {
    return {static_cast<float>(vp.size.x), static_cast<float>(vp.size.y)};
}

void centered_message(const char* id, sf::IntRect vp, float y_fraction, float scale,
                      const char* line1, const char* line2) {
    const ImVec2 o = origin(vp);
    const ImVec2 s = size(vp);
    ImGui::SetNextWindowPos({o.x + s.x * 0.5F, o.y + s.y * y_fraction}, ImGuiCond_Always,
                            {0.5F, 0.5F});
    ImGui::Begin(id, nullptr, kOverlayFlags);
    ImGui::SetWindowFontScale(scale);
    ImGui::TextUnformatted(line1);
    if (line2 != nullptr) {
        ImGui::SetWindowFontScale(scale * 0.5F);
        ImGui::TextUnformatted(line2);
    }
    ImGui::End();
}

}  // namespace

void draw_hud(const asteroids::Game& game, sf::IntRect viewport) {
    const ImVec2 o = origin(viewport);
    ImGui::SetNextWindowPos({o.x + 14.0F, o.y + 8.0F}, ImGuiCond_Always);
    ImGui::Begin("##score", nullptr, kOverlayFlags);
    ImGui::SetWindowFontScale(2.4F);
    ImGui::Text("%06d", game.score());
    ImGui::SetWindowFontScale(1.2F);
    ImGui::Text("WAVE %d", game.wave());
    ImGui::End();

    if (game.phase() == asteroids::Phase::kGameOver) {
        centered_message("##gameover", viewport, 0.42F, 4.0F, "GAME OVER",
                         "Press Enter to play again");
    } else if (game.tick() < kShowControlsTicks) {
        centered_message("##controls", viewport, 0.75F, 1.4F,
                         "Arrows / WASD to fly    Space to fire",
                         "P pause    F1 debug panel    Esc quit");
    }
}

void draw_debug_panel(asteroids::Game& game, DebugState& state, sf::IntRect viewport) {
    if (!state.visible) {
        return;
    }
    const ImVec2 o = origin(viewport);
    const ImVec2 s = size(viewport);
    ImGui::SetNextWindowPos({o.x + s.x - 14.0F, o.y + 14.0F}, ImGuiCond_FirstUseEver, {1.0F, 0.0F});
    ImGui::SetNextWindowBgAlpha(0.85F);
    if (!ImGui::Begin("Debug (F1)", &state.visible, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("%.0f FPS   %.2f ms/frame", static_cast<double>(io.Framerate),
                1000.0 / static_cast<double>(io.Framerate));
    ImGui::Text("Simulation ticks this frame: %d", state.ticks_last_frame);
    ImGui::Text("Tick %llu   Seed %llu", static_cast<unsigned long long>(game.tick()),
                static_cast<unsigned long long>(game.seed()));

    ImGui::SeparatorText("Pools (fixed capacity)");
    ImGui::Text("Asteroids  %3d / %d", static_cast<int>(game.asteroids().size()),
                static_cast<int>(asteroids::config::kMaxAsteroids));
    ImGui::Text("Bullets    %3d / %d", static_cast<int>(game.bullets().size()),
                static_cast<int>(asteroids::config::kMaxBullets));
    ImGui::Text("Particles  %3d / %d", static_cast<int>(game.particles().size()),
                static_cast<int>(asteroids::config::kMaxParticles));

    ImGui::SeparatorText("Collision broad phase");
    const auto& stats = game.collision_stats();
    ImGui::Text("Grid checks         %u", stats.grid_checks);
    ImGui::Text("Brute-force checks  %u", stats.brute_force_checks);
    if (stats.brute_force_checks > 0) {
        ImGui::Text("Work saved          %.0f%%",
                    100.0 * (1.0 - static_cast<double>(stats.grid_checks) /
                                       static_cast<double>(stats.brute_force_checks)));
    }
    ImGui::Checkbox("Show collision grid", &state.render.show_grid);

    ImGui::SeparatorText("Controls");
    ImGui::Checkbox("Pause (P)", &state.paused);
    ImGui::SliderFloat("Time scale", &state.time_scale, 0.1F, 2.0F, "%.2fx");
    bool god = game.god_mode();
    if (ImGui::Checkbox("God mode", &god)) {
        game.set_god_mode(god);
    }
    if (ImGui::Button("Spawn asteroid")) {
        const float heading = static_cast<float>(game.tick() % 628U) / 100.0F;
        game.spawn_asteroid({0.0F, 0.0F}, asteroids::from_angle(heading) * 70.0F,
                            asteroids::AsteroidSize::kLarge);
    }
    ImGui::SameLine();
    if (ImGui::Button("Skip wave")) {
        game.clear_asteroids();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restart")) {
        game.restart();
    }

    ImGui::Text("State hash %016llx", static_cast<unsigned long long>(game.state_hash()));
    ImGui::End();
}

}  // namespace app
