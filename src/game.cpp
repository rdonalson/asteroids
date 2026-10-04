#include "asteroids/game.hpp"

#include <algorithm>
#include <bit>
#include <numbers>

namespace asteroids {

namespace {

using namespace config;

constexpr float kTwoPi = 2.0F * std::numbers::pi_v<float>;
constexpr Vec2 kWorldCenter{kWorldWidth / 2.0F, kWorldHeight / 2.0F};

constexpr AsteroidSize smaller(AsteroidSize size) noexcept {
    return size == AsteroidSize::kLarge ? AsteroidSize::kMedium : AsteroidSize::kSmall;
}

class Fnv1a {
public:
    void add(std::uint64_t value) noexcept {
        for (int i = 0; i < 8; ++i) {
            hash_ ^= (value >> (8 * i)) & 0xFFU;
            hash_ *= 0x100000001B3ULL;
        }
    }
    void add(float value) noexcept {
        add(static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(value)));
    }
    void add(Vec2 v) noexcept {
        add(v.x);
        add(v.y);
    }
    void add(int value) noexcept {
        add(static_cast<std::uint64_t>(static_cast<std::uint32_t>(value)));
    }
    [[nodiscard]] std::uint64_t value() const noexcept { return hash_; }

private:
    std::uint64_t hash_{0xCBF29CE484222325ULL};
};

}  // namespace

Game::Game(std::uint64_t seed) noexcept : seed_(seed), rng_(seed) { restart(); }

void Game::restart() noexcept {
    ship_ = Ship{};
    ship_.pos = kWorldCenter;
    ship_.invulnerable = kInvulnerableTime;
    asteroids_.clear();
    bullets_.clear();
    particles_.clear();
    score_ = 0;
    lives_ = kStartLives;
    wave_ = 0;
    next_extra_life_ = kExtraLifeEvery;
    phase_ = Phase::kPlaying;
    spawn_wave();
}

void Game::step(const Input& input) noexcept {
    ++tick_;
    if (phase_ == Phase::kPlaying) {
        update_ship(input);
    }
    update_bullets();
    update_asteroids();
    update_particles();
    resolve_collisions();
    if (phase_ == Phase::kPlaying) {
        try_respawn();
        update_waves();
    }
}

void Game::update_ship(const Input& input) noexcept {
    Ship& s = ship_;
    s.thrusting = false;
    if (!s.alive) {
        return;
    }

    if (input.rotate_left) {
        s.angle -= kShipTurnRate * kDt;
    }
    if (input.rotate_right) {
        s.angle += kShipTurnRate * kDt;
    }
    s.angle = wrap(s.angle, kTwoPi);

    if (input.thrust) {
        s.vel += from_angle(s.angle) * (kShipThrust * kDt);
        s.thrusting = true;
    }
    s.vel *= 1.0F - kShipDrag * kDt;
    const float speed = length(s.vel);
    if (speed > kShipMaxSpeed) {
        s.vel *= kShipMaxSpeed / speed;
    }
    s.pos = wrap_position(s.pos + s.vel * kDt);

    s.fire_cooldown = std::max(0.0F, s.fire_cooldown - kDt);
    s.invulnerable = std::max(0.0F, s.invulnerable - kDt);

    if (input.fire && s.fire_cooldown <= 0.0F) {
        if (Bullet* b = bullets_.spawn()) {
            const Vec2 dir = from_angle(s.angle);
            b->pos = s.pos + dir * kShipRadius;
            b->vel = s.vel + dir * kBulletSpeed;
            b->life = kBulletLife;
            s.fire_cooldown = kFireCooldown;
        }
    }
}

void Game::update_bullets() noexcept {
    for (Bullet& b : bullets_) {
        b.pos = wrap_position(b.pos + b.vel * kDt);
        b.life -= kDt;
    }
    bullets_.remove_if([](const Bullet& b) { return b.life <= 0.0F; });
}

void Game::update_asteroids() noexcept {
    for (Asteroid& a : asteroids_) {
        a.pos = wrap_position(a.pos + a.vel * kDt);
        a.angle = wrap(a.angle + a.spin * kDt, kTwoPi);
    }
}

void Game::update_particles() noexcept {
    for (Particle& p : particles_) {
        p.pos = wrap_position(p.pos + p.vel * kDt);
        p.vel *= 1.0F - 1.5F * kDt;
        p.life -= kDt;
    }
    particles_.remove_if([](const Particle& p) { return p.life <= 0.0F; });
}

void Game::resolve_collisions() noexcept {
    grid_.build(asteroids_.size(), [this](std::size_t i) { return asteroids_[i].pos; });

    stats_ = {};
    const std::size_t asteroid_count = asteroids_.size();
    const bool ship_vulnerable =
        phase_ == Phase::kPlaying && ship_.alive && ship_.invulnerable <= 0.0F && !god_mode_;
    stats_.brute_force_checks = static_cast<std::uint32_t>(
        asteroid_count * (bullets_.size() + (ship_vulnerable ? 1U : 0U)));

    // Bullets vs asteroids: each bullet destroys at most one asteroid.
    for (Bullet& b : bullets_) {
        grid_.for_each_near(b.pos, [&](std::size_t i) {
            Asteroid& a = asteroids_[i];
            if (b.spent || a.destroyed) {
                return;
            }
            ++stats_.grid_checks;
            if (circles_overlap(b.pos, kBulletRadius, a.pos, radius_of(a.size))) {
                b.spent = true;
                a.destroyed = true;
                add_score(score_of(a.size));
            }
        });
    }

    // Ship vs asteroids.
    if (ship_vulnerable) {
        grid_.for_each_near(ship_.pos, [&](std::size_t i) {
            Asteroid& a = asteroids_[i];
            if (!ship_.alive || a.destroyed) {
                return;
            }
            ++stats_.grid_checks;
            if (circles_overlap(ship_.pos, kShipRadius, a.pos, radius_of(a.size))) {
                a.destroyed = true;
                add_score(score_of(a.size));
                kill_ship();
            }
        });
    }

    // Split destroyed rocks. Children are appended past asteroid_count, so
    // this loop only visits rocks that existed during the collision pass.
    for (std::size_t i = 0; i < asteroid_count; ++i) {
        if (asteroids_[i].destroyed) {
            destroy_asteroid(asteroids_[i]);
        }
    }
    asteroids_.remove_if([](const Asteroid& a) { return a.destroyed; });
    bullets_.remove_if([](const Bullet& b) { return b.spent; });
}

void Game::destroy_asteroid(Asteroid& asteroid) noexcept {
    // Copy first: spawning children may not move the parent today, but the
    // pool contract only promises pointer stability until the next removal.
    const Asteroid parent = asteroid;
    const float r = radius_of(parent.size);
    spawn_explosion(parent.pos, static_cast<int>(r * 0.6F), 60.0F + r * 2.0F);

    if (parent.size == AsteroidSize::kSmall) {
        return;
    }
    const float parent_speed = length(parent.vel);
    for (int k = 0; k < 2; ++k) {
        const Vec2 dir = from_angle(rng_.range(0.0F, kTwoPi));
        const float speed = parent_speed * 1.15F + rng_.range(20.0F, 60.0F);
        spawn_asteroid(parent.pos, dir * speed, smaller(parent.size));
    }
}

Asteroid* Game::spawn_asteroid(Vec2 pos, Vec2 vel, AsteroidSize size) noexcept {
    Asteroid* a = asteroids_.spawn();
    if (a == nullptr) {
        return nullptr;  // pool full: skip the rock rather than allocate
    }
    a->pos = wrap_position(pos);
    a->vel = vel;
    a->size = size;
    a->angle = rng_.range(0.0F, kTwoPi);
    a->spin = rng_.range(-1.2F, 1.2F);
    a->shape_seed = rng_.next_u32();
    return a;
}

void Game::place_ship(Vec2 pos, Vec2 vel, float angle) noexcept {
    ship_.pos = wrap_position(pos);
    ship_.vel = vel;
    ship_.angle = wrap(angle, kTwoPi);
}

void Game::kill_ship() noexcept {
    ship_.alive = false;
    ship_.thrusting = false;
    ship_.respawn_timer = kRespawnDelay;
    spawn_explosion(ship_.pos, 40, 160.0F);
    --lives_;
    if (lives_ <= 0) {
        phase_ = Phase::kGameOver;
    }
}

void Game::try_respawn() noexcept {
    if (ship_.alive) {
        return;
    }
    ship_.respawn_timer -= kDt;
    if (ship_.respawn_timer > 0.0F) {
        return;
    }
    // Wait until the centre is clear so the player never respawns into a rock.
    const bool blocked = std::any_of(asteroids_.begin(), asteroids_.end(), [](const Asteroid& a) {
        return circles_overlap(kWorldCenter, kRespawnClearRadius, a.pos, radius_of(a.size));
    });
    if (blocked) {
        return;
    }
    ship_ = Ship{};
    ship_.pos = kWorldCenter;
    ship_.invulnerable = kInvulnerableTime;
}

void Game::update_waves() noexcept {
    if (!asteroids_.empty()) {
        wave_timer_ = kWaveDelay;
        return;
    }
    wave_timer_ -= kDt;
    if (wave_timer_ <= 0.0F) {
        spawn_wave();
    }
}

void Game::spawn_wave() noexcept {
    ++wave_;
    wave_timer_ = kWaveDelay;
    const int count = std::min(kFirstWaveCount + wave_ - 1, kMaxWaveCount);
    const float speed_scale = 1.0F + kWaveSpeedup * static_cast<float>(wave_ - 1);

    for (int i = 0; i < count; ++i) {
        // Pick a spot away from the ship; give up after a few tries rather than loop forever.
        Vec2 pos{};
        for (int attempt = 0; attempt < 16; ++attempt) {
            pos = {rng_.range(0.0F, kWorldWidth), rng_.range(0.0F, kWorldHeight)};
            if (!circles_overlap(pos, kSafeSpawnDistance, ship_.pos, 0.0F)) {
                break;
            }
        }
        const Vec2 dir = from_angle(rng_.range(0.0F, kTwoPi));
        const float speed = rng_.range(kAsteroidMinSpeed, kAsteroidMaxSpeed) * speed_scale;
        spawn_asteroid(pos, dir * speed, AsteroidSize::kLarge);
    }
}

void Game::spawn_explosion(Vec2 pos, int count, float speed) noexcept {
    for (int i = 0; i < count; ++i) {
        Particle* p = particles_.spawn();
        if (p == nullptr) {
            return;
        }
        p->pos = pos;
        p->vel = from_angle(rng_.range(0.0F, kTwoPi)) * rng_.range(0.2F * speed, speed);
        p->life = rng_.range(0.4F * kParticleLife, kParticleLife);
    }
}

void Game::add_score(int points) noexcept {
    if (phase_ != Phase::kPlaying) {
        return;
    }
    score_ += points;
    while (score_ >= next_extra_life_) {
        ++lives_;
        next_extra_life_ += kExtraLifeEvery;
    }
}

std::uint64_t Game::state_hash() const noexcept {
    Fnv1a h;
    h.add(tick_);
    h.add(ship_.pos);
    h.add(ship_.vel);
    h.add(ship_.angle);
    h.add(ship_.alive ? 1 : 0);
    for (const Asteroid& a : asteroids_) {
        h.add(a.pos);
        h.add(a.vel);
        h.add(static_cast<int>(a.size));
    }
    for (const Bullet& b : bullets_) {
        h.add(b.pos);
    }
    h.add(score_);
    h.add(lives_);
    h.add(wave_);
    return h.value();
}

}  // namespace asteroids
