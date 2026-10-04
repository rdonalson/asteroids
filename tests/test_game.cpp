#include <gtest/gtest.h>

#include <cmath>

#include "asteroids/game.hpp"

using namespace asteroids;

namespace {

constexpr Vec2 kCenter{config::kWorldWidth / 2.0F, config::kWorldHeight / 2.0F};

/// Ticks needed for a timer to run out. Rounds up and adds one tick of margin,
/// because subtracting 1/60 repeatedly in float lands a hair above zero.
int ticks_for(float seconds) { return static_cast<int>(std::ceil(seconds / config::kDt)) + 1; }

void run(Game& game, int ticks, const Input& input = {}) {
    for (int i = 0; i < ticks; ++i) {
        game.step(input);
    }
}

/// A game with no rocks and the ship parked mid-screen, ready for a scenario.
Game empty_arena() {
    Game game(1U);
    game.clear_asteroids();
    game.place_ship(kCenter, {}, 0.0F);
    game.set_invulnerable(0.0F);
    return game;
}

/// Scripted "player" used by the determinism tests.
Input scripted_input(std::uint64_t t) {
    return {.rotate_left = (t / 90) % 2 == 0,
            .rotate_right = false,
            .thrust = t % 120 < 30,
            .fire = true};
}

}  // namespace

TEST(Game, StartsWithFirstWave) {
    const Game game(1U);
    EXPECT_EQ(game.wave(), 1);
    EXPECT_EQ(game.lives(), config::kStartLives);
    EXPECT_EQ(game.score(), 0);
    EXPECT_EQ(game.asteroids().size(), static_cast<std::size_t>(config::kFirstWaveCount));
    EXPECT_EQ(game.phase(), Phase::kPlaying);
}

TEST(Game, FirstWaveSpawnsAwayFromShip) {
    for (std::uint64_t seed = 0; seed < 50; ++seed) {
        const Game game(seed);
        for (const Asteroid& a : game.asteroids()) {
            EXPECT_FALSE(circles_overlap(a.pos, 0.0F, game.ship().pos, config::kSafeSpawnDistance))
                << "seed " << seed;
        }
    }
}

TEST(Game, RotationFollowsInput) {
    Game game = empty_arena();
    run(game, 10, {.rotate_right = true});
    EXPECT_NEAR(game.ship().angle, 10 * config::kShipTurnRate * config::kDt, 1e-4F);
}

TEST(Game, ThrustAcceleratesAndSpeedIsCapped) {
    Game game = empty_arena();
    run(game, 1, {.thrust = true});
    EXPECT_LT(game.ship().vel.y, 0.0F);  // angle 0 = up the screen
    run(game, 60 * 20, {.thrust = true});
    EXPECT_LE(length(game.ship().vel), config::kShipMaxSpeed + 1e-3F);
}

TEST(Game, DragSlowsShipDown) {
    Game game = empty_arena();
    game.place_ship(kCenter, {200.0F, 0.0F}, 0.0F);
    run(game, 60);
    EXPECT_LT(game.ship().vel.x, 200.0F);
    EXPECT_GT(game.ship().vel.x, 0.0F);
}

TEST(Game, ShipWrapsAroundScreenEdge) {
    Game game = empty_arena();
    game.place_ship({config::kWorldWidth - 1.0F, 100.0F}, {300.0F, 0.0F}, 0.0F);
    run(game, 2);
    EXPECT_LT(game.ship().pos.x, 20.0F);
}

TEST(Game, FireCooldownLimitsRateOfFire) {
    Game game = empty_arena();
    run(game, 1, {.fire = true});
    EXPECT_EQ(game.bullets().size(), 1U);
    run(game, 5, {.fire = true});  // 5 ticks < cooldown
    EXPECT_EQ(game.bullets().size(), 1U);
    run(game, ticks_for(config::kFireCooldown), {.fire = true});
    EXPECT_EQ(game.bullets().size(), 2U);
}

TEST(Game, BulletsExpire) {
    Game game = empty_arena();
    run(game, 1, {.fire = true});
    run(game, ticks_for(config::kBulletLife));
    EXPECT_TRUE(game.bullets().empty());
}

TEST(Game, BulletCountNeverExceedsPool) {
    Game game = empty_arena();
    for (int i = 0; i < 600; ++i) {
        game.step({.rotate_left = true, .fire = true});
        ASSERT_LE(game.bullets().size(), config::kMaxBullets);
    }
}

TEST(Game, ShootingLargeAsteroidSplitsItInTwo) {
    Game game = empty_arena();
    game.spawn_asteroid({kCenter.x, kCenter.y - 120.0F}, {}, AsteroidSize::kLarge);

    run(game, 1, {.fire = true});
    run(game, 30);

    ASSERT_EQ(game.asteroids().size(), 2U);
    for (const Asteroid& a : game.asteroids()) {
        EXPECT_EQ(a.size, AsteroidSize::kMedium);
    }
    EXPECT_EQ(game.score(), config::kLargeScore);
    EXPECT_FALSE(game.particles().empty());
}

TEST(Game, SmallAsteroidIsDestroyedCompletely) {
    Game game = empty_arena();
    game.spawn_asteroid({kCenter.x, kCenter.y - 120.0F}, {}, AsteroidSize::kSmall);

    run(game, 1, {.fire = true});
    run(game, 30);

    EXPECT_TRUE(game.asteroids().empty());
    EXPECT_EQ(game.score(), config::kSmallScore);
}

TEST(Game, CollisionCostsALifeThenShipRespawnsProtected) {
    Game game = empty_arena();
    game.spawn_asteroid(kCenter, {}, AsteroidSize::kSmall);
    run(game, 1);

    EXPECT_FALSE(game.ship().alive);
    EXPECT_EQ(game.lives(), config::kStartLives - 1);

    run(game, ticks_for(config::kRespawnDelay));
    EXPECT_TRUE(game.ship().alive);
    EXPECT_GT(game.ship().invulnerable, 0.0F);
}

TEST(Game, InvulnerableShipIgnoresAsteroids) {
    Game game = empty_arena();
    game.set_invulnerable(1.0F);
    game.spawn_asteroid(kCenter, {}, AsteroidSize::kLarge);
    run(game, 10);
    EXPECT_TRUE(game.ship().alive);
    EXPECT_EQ(game.lives(), config::kStartLives);
}

TEST(Game, LosingLastLifeEndsGame) {
    Game game = empty_arena();
    for (int life = 0; life < config::kStartLives; ++life) {
        game.place_ship(kCenter, {}, 0.0F);
        game.set_invulnerable(0.0F);
        game.spawn_asteroid(kCenter, {}, AsteroidSize::kSmall);
        run(game, 1);
        game.clear_asteroids();
        run(game, ticks_for(config::kRespawnDelay));
    }
    EXPECT_EQ(game.lives(), 0);
    EXPECT_EQ(game.phase(), Phase::kGameOver);

    game.restart();
    EXPECT_EQ(game.phase(), Phase::kPlaying);
    EXPECT_EQ(game.lives(), config::kStartLives);
    EXPECT_EQ(game.score(), 0);
}

TEST(Game, ClearingWaveStartsBiggerNextWave) {
    Game game = empty_arena();
    run(game, ticks_for(config::kWaveDelay));
    EXPECT_EQ(game.wave(), 2);
    EXPECT_EQ(game.asteroids().size(), static_cast<std::size_t>(config::kFirstWaveCount + 1));
}

TEST(Game, AsteroidPoolOverflowIsHandledGracefully) {
    Game game = empty_arena();
    game.set_god_mode(true);
    while (game.spawn_asteroid({100.0F, 100.0F}, {}, AsteroidSize::kLarge) != nullptr) {
    }
    EXPECT_EQ(game.asteroids().size(), config::kMaxAsteroids);
    EXPECT_EQ(game.spawn_asteroid({}, {}, AsteroidSize::kLarge), nullptr);
    run(game, 600, {.rotate_left = true, .fire = true});  // splits into a full pool
    EXPECT_LE(game.asteroids().size(), config::kMaxAsteroids);
}

TEST(Game, GridMatchesBruteForceResult) {
    // Bullets in the debug stats: the grid should do far fewer checks than all-pairs.
    Game game = empty_arena();
    game.set_god_mode(true);
    for (int i = 0; i < 60; ++i) {
        game.spawn_asteroid({static_cast<float>(i) * 20.0F, static_cast<float>(i % 9) * 80.0F}, {},
                            AsteroidSize::kLarge);
    }
    run(game, 30, {.rotate_left = true, .fire = true});
    const auto& stats = game.collision_stats();
    EXPECT_GT(stats.brute_force_checks, 0U);
    EXPECT_LT(stats.grid_checks, stats.brute_force_checks);
}

TEST(Game, SameSeedAndInputGiveIdenticalGames) {
    Game a(2024U);
    Game b(2024U);
    for (std::uint64_t t = 0; t < 60 * 60; ++t) {  // one minute of play
        const Input in = scripted_input(t);
        a.step(in);
        b.step(in);
        ASSERT_EQ(a.state_hash(), b.state_hash()) << "diverged at tick " << t;
    }
    EXPECT_GT(a.score(), 0);  // the script actually played the game
}

TEST(Game, DifferentSeedsGiveDifferentGames) {
    Game a(1U);
    Game b(2U);
    for (std::uint64_t t = 0; t < 60; ++t) {
        a.step(scripted_input(t));
        b.step(scripted_input(t));
    }
    EXPECT_NE(a.state_hash(), b.state_hash());
}
