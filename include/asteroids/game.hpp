#pragma once

#include <cstdint>
#include <span>

#include "asteroids/config.hpp"
#include "asteroids/fixed_pool.hpp"
#include "asteroids/rng.hpp"
#include "asteroids/spatial_grid.hpp"
#include "asteroids/vec2.hpp"

namespace asteroids {

/// Player input for one simulation tick. The game core never reads the
/// keyboard itself, so tests can drive it with scripted input.
struct Input {
    bool rotate_left{};
    bool rotate_right{};
    bool thrust{};
    bool fire{};
};

enum class AsteroidSize : std::uint8_t { kSmall, kMedium, kLarge };

[[nodiscard]] constexpr float radius_of(AsteroidSize size) noexcept {
    switch (size) {
        case AsteroidSize::kLarge:
            return config::kLargeRadius;
        case AsteroidSize::kMedium:
            return config::kMediumRadius;
        case AsteroidSize::kSmall:
            break;
    }
    return config::kSmallRadius;
}

[[nodiscard]] constexpr int score_of(AsteroidSize size) noexcept {
    switch (size) {
        case AsteroidSize::kLarge:
            return config::kLargeScore;
        case AsteroidSize::kMedium:
            return config::kMediumScore;
        case AsteroidSize::kSmall:
            break;
    }
    return config::kSmallScore;
}

struct Ship {
    Vec2 pos{};
    Vec2 vel{};
    float angle{};
    float fire_cooldown{};
    float invulnerable{};   ///< seconds of spawn protection left
    float respawn_timer{};  ///< seconds until respawn while dead
    bool alive{true};
    bool thrusting{};  ///< for the renderer's engine flame
};

struct Asteroid {
    Vec2 pos{};
    Vec2 vel{};
    float angle{};
    float spin{};
    AsteroidSize size{AsteroidSize::kLarge};
    std::uint32_t shape_seed{};  ///< lets the renderer give each rock its own outline
    bool destroyed{};
};

struct Bullet {
    Vec2 pos{};
    Vec2 vel{};
    float life{};
    bool spent{};
};

struct Particle {
    Vec2 pos{};
    Vec2 vel{};
    float life{};
};

enum class Phase : std::uint8_t { kPlaying, kGameOver };

/// Broad-phase statistics from the last tick, shown in the debug panel.
struct CollisionStats {
    std::uint32_t grid_checks{};         ///< exact circle tests actually performed
    std::uint32_t brute_force_checks{};  ///< tests an all-pairs approach would need
};

/// The complete, rendering-independent game simulation.
///
/// Everything lives in fixed-size pools inside this object: after construction,
/// step() never touches the heap (verified by a unit test).
class Game {
public:
    using AsteroidPool = FixedPool<Asteroid, config::kMaxAsteroids>;
    using BulletPool = FixedPool<Bullet, config::kMaxBullets>;
    using ParticlePool = FixedPool<Particle, config::kMaxParticles>;

    explicit Game(std::uint64_t seed) noexcept;

    /// Advance the simulation by exactly one fixed step (config::kDt).
    void step(const Input& input) noexcept;

    /// Start a fresh game (keeps the random sequence going).
    void restart() noexcept;

    [[nodiscard]] const Ship& ship() const noexcept { return ship_; }
    [[nodiscard]] std::span<const Asteroid> asteroids() const noexcept {
        return asteroids_.items();
    }
    [[nodiscard]] std::span<const Bullet> bullets() const noexcept { return bullets_.items(); }
    [[nodiscard]] std::span<const Particle> particles() const noexcept {
        return particles_.items();
    }

    [[nodiscard]] int score() const noexcept { return score_; }
    [[nodiscard]] int lives() const noexcept { return lives_; }
    [[nodiscard]] int wave() const noexcept { return wave_; }
    [[nodiscard]] Phase phase() const noexcept { return phase_; }
    [[nodiscard]] std::uint64_t tick() const noexcept { return tick_; }
    [[nodiscard]] std::uint64_t seed() const noexcept { return seed_; }
    [[nodiscard]] const CollisionStats& collision_stats() const noexcept { return stats_; }

    /// FNV-1a hash of the full simulation state, used to prove determinism.
    [[nodiscard]] std::uint64_t state_hash() const noexcept;

    // --- Sandbox controls, used by unit tests and the in-game debug panel ---
    void clear_asteroids() noexcept { asteroids_.clear(); }
    Asteroid* spawn_asteroid(Vec2 pos, Vec2 vel, AsteroidSize size) noexcept;
    void place_ship(Vec2 pos, Vec2 vel, float angle) noexcept;
    void set_invulnerable(float seconds) noexcept { ship_.invulnerable = seconds; }
    void set_god_mode(bool enabled) noexcept { god_mode_ = enabled; }
    [[nodiscard]] bool god_mode() const noexcept { return god_mode_; }

private:
    void update_ship(const Input& input) noexcept;
    void update_bullets() noexcept;
    void update_asteroids() noexcept;
    void update_particles() noexcept;
    void resolve_collisions() noexcept;
    void destroy_asteroid(Asteroid& asteroid) noexcept;
    void kill_ship() noexcept;
    void try_respawn() noexcept;
    void update_waves() noexcept;
    void spawn_wave() noexcept;
    void spawn_explosion(Vec2 pos, int count, float speed) noexcept;
    void add_score(int points) noexcept;

    std::uint64_t seed_;
    Rng rng_;
    Ship ship_{};
    AsteroidPool asteroids_{};
    BulletPool bullets_{};
    ParticlePool particles_{};
    SpatialGrid<config::kGridCellsX, config::kGridCellsY, config::kMaxAsteroids> grid_{};
    CollisionStats stats_{};

    int score_{0};
    int lives_{config::kStartLives};
    int wave_{0};
    int next_extra_life_{config::kExtraLifeEvery};
    float wave_timer_{0.0F};
    Phase phase_{Phase::kPlaying};
    std::uint64_t tick_{0};
    bool god_mode_{false};
};

}  // namespace asteroids
