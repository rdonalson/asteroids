#pragma once

#include <cstdint>

namespace asteroids {

/// PCG32 random number generator (pcg-random.org).
///
/// Why not <random>? std::mt19937 itself is portable, but distributions such as
/// std::uniform_real_distribution are implementation-defined, so MSVC and GCC
/// produce different games from the same seed. Owning the whole pipeline keeps
/// the simulation reproducible, which is what makes the determinism tests possible.
class Rng {
public:
    explicit constexpr Rng(std::uint64_t seed, std::uint64_t sequence = 54U) noexcept
        : inc_((sequence << 1U) | 1U) {
        (void)next_u32();
        state_ += seed;
        (void)next_u32();
    }

    [[nodiscard]] constexpr std::uint32_t next_u32() noexcept {
        const std::uint64_t old = state_;
        state_ = old * 6364136223846793005ULL + inc_;
        const auto xorshifted = static_cast<std::uint32_t>(((old >> 18U) ^ old) >> 27U);
        const auto rot = static_cast<std::uint32_t>(old >> 59U);
        return (xorshifted >> rot) | (xorshifted << ((~rot + 1U) & 31U));
    }

    /// Uniform float in [0, 1).
    [[nodiscard]] constexpr float next_float() noexcept {
        return static_cast<float>(next_u32() >> 8U) * (1.0F / 16'777'216.0F);
    }

    /// Uniform float in [lo, hi).
    [[nodiscard]] constexpr float range(float lo, float hi) noexcept {
        return lo + (hi - lo) * next_float();
    }

private:
    std::uint64_t state_{0};
    std::uint64_t inc_;
};

}  // namespace asteroids
