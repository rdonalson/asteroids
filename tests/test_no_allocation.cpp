// Proves the "no heap allocation during gameplay" rule by replacing the global
// allocator in this test binary and counting calls while the game runs.

#include <gtest/gtest.h>

#include <atomic>
#include <cstdlib>
#include <memory>
#include <new>

#include "asteroids/game.hpp"

// AddressSanitizer installs its own global allocator, which conflicts with
// ours, so under ASan the test is skipped (it still runs in normal builds).
#if defined(__SANITIZE_ADDRESS__)
#define ASTEROIDS_ASAN 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ASTEROIDS_ASAN 1
#endif
#endif

#ifndef ASTEROIDS_ASAN

namespace {
std::atomic<bool> g_counting{false};
std::atomic<std::size_t> g_allocations{0};
}  // namespace

// NOLINTBEGIN(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
void* operator new(std::size_t size) {
    if (g_counting.load(std::memory_order_relaxed)) {
        g_allocations.fetch_add(1, std::memory_order_relaxed);
    }
    if (void* p = std::malloc(size == 0 ? 1 : size)) {
        return p;
    }
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t /*size*/) noexcept { std::free(p); }
// NOLINTEND(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)

TEST(NoAllocation, TenMinutesOfGameplayNeverTouchesTheHeap) {
    auto game = std::make_unique<asteroids::Game>(99U);  // allocated before counting starts

    g_allocations = 0;
    g_counting = true;
    for (std::uint64_t t = 0; t < 60ULL * 60 * 10; ++t) {
        game->step({.rotate_left = (t / 70) % 3 == 0,
                    .rotate_right = (t / 70) % 3 == 1,
                    .thrust = t % 100 < 20,
                    .fire = true});
        if (game->phase() == asteroids::Phase::kGameOver) {
            game->restart();
        }
    }
    g_counting = false;

    EXPECT_EQ(g_allocations.load(), 0U);
    EXPECT_GT(game->tick(), 0U);
}

#else

TEST(NoAllocation, TenMinutesOfGameplayNeverTouchesTheHeap) {
    GTEST_SKIP() << "AddressSanitizer replaces the global allocator; run in a normal build";
}

#endif
