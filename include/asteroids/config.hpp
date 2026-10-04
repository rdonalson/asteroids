#pragma once

#include <cstddef>

/// All gameplay tuning in one place. Units are pixels and seconds.
namespace asteroids::config {

// World (the renderer scales this to any window size).
inline constexpr float kWorldWidth = 1280.0F;
inline constexpr float kWorldHeight = 720.0F;

// Fixed simulation step: the game always updates at 60 Hz regardless of frame rate.
inline constexpr float kDt = 1.0F / 60.0F;

// Ship
inline constexpr float kShipRadius = 12.0F;
inline constexpr float kShipTurnRate = 4.5F;  // rad/s
inline constexpr float kShipThrust = 320.0F;  // px/s^2
inline constexpr float kShipDrag = 0.5F;      // fraction of speed lost per second
inline constexpr float kShipMaxSpeed = 420.0F;
inline constexpr float kRespawnDelay = 2.0F;
inline constexpr float kInvulnerableTime = 2.5F;
inline constexpr float kRespawnClearRadius = 120.0F;
inline constexpr int kStartLives = 3;
inline constexpr int kExtraLifeEvery = 10'000;

// Bullets
inline constexpr float kFireCooldown = 0.18F;
inline constexpr float kBulletSpeed = 560.0F;
inline constexpr float kBulletLife = 0.9F;
inline constexpr float kBulletRadius = 2.0F;

// Asteroids
inline constexpr float kLargeRadius = 40.0F;
inline constexpr float kMediumRadius = 22.0F;
inline constexpr float kSmallRadius = 12.0F;
inline constexpr int kLargeScore = 20;
inline constexpr int kMediumScore = 50;
inline constexpr int kSmallScore = 100;
inline constexpr float kAsteroidMinSpeed = 40.0F;
inline constexpr float kAsteroidMaxSpeed = 90.0F;
inline constexpr float kWaveSpeedup = 0.08F;  // +8% speed per wave
inline constexpr int kFirstWaveCount = 4;
inline constexpr int kMaxWaveCount = 11;
inline constexpr float kWaveDelay = 2.0F;
inline constexpr float kSafeSpawnDistance = 220.0F;

// Particles (explosion debris, purely visual but still deterministic)
inline constexpr float kParticleLife = 0.8F;

// Fixed pool capacities: nothing is allocated on the heap during gameplay.
inline constexpr std::size_t kMaxAsteroids = 96;
inline constexpr std::size_t kMaxBullets = 8;
inline constexpr std::size_t kMaxParticles = 512;

// Collision broad-phase grid: 16 x 9 cells of 80 x 80 px.
inline constexpr std::size_t kGridCellsX = 16;
inline constexpr std::size_t kGridCellsY = 9;

// The grid only searches the 3x3 cells around an object, so the largest
// possible collision distance must fit inside one cell.
static_assert(kWorldWidth / kGridCellsX >= kLargeRadius + kShipRadius);
static_assert(kWorldHeight / kGridCellsY >= kLargeRadius + kShipRadius);

}  // namespace asteroids::config
