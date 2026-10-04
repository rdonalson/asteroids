#include <gtest/gtest.h>

#include <set>

#include "asteroids/fixed_pool.hpp"
#include "asteroids/rng.hpp"
#include "asteroids/vec2.hpp"

using namespace asteroids;

// ---------------------------------------------------------------- Vec2 / wrap

TEST(Wrap, KeepsValuesInsideRange) {
    EXPECT_FLOAT_EQ(wrap(10.0F, 100.0F), 10.0F);
    EXPECT_FLOAT_EQ(wrap(105.0F, 100.0F), 5.0F);
    EXPECT_FLOAT_EQ(wrap(-5.0F, 100.0F), 95.0F);
    EXPECT_FLOAT_EQ(wrap(100.0F, 100.0F), 0.0F);
}

TEST(Wrap, TinyNegativeNeverReturnsMax) {
    const float r = wrap(-1e-9F, 100.0F);
    EXPECT_GE(r, 0.0F);
    EXPECT_LT(r, 100.0F);
}

TEST(WrappedDelta, TakesShortWayAcrossEdge) {
    const Vec2 a{5.0F, 360.0F};
    const Vec2 b{config::kWorldWidth - 5.0F, 360.0F};
    const Vec2 d = wrapped_delta(a, b);
    EXPECT_FLOAT_EQ(d.x, -10.0F);
    EXPECT_FLOAT_EQ(d.y, 0.0F);
}

TEST(CirclesOverlap, DetectsOverlapAcrossCorner) {
    // Two circles in opposite corners touch through the wrap-around.
    EXPECT_TRUE(circles_overlap({2.0F, 2.0F}, 5.0F,
                                {config::kWorldWidth - 2.0F, config::kWorldHeight - 2.0F}, 5.0F));
    EXPECT_FALSE(circles_overlap({100.0F, 100.0F}, 5.0F, {200.0F, 100.0F}, 5.0F));
}

TEST(FromAngle, ZeroPointsUpScreen) {
    const Vec2 up = from_angle(0.0F);
    EXPECT_NEAR(up.x, 0.0F, 1e-6F);
    EXPECT_NEAR(up.y, -1.0F, 1e-6F);
}

static_assert(Vec2{1.0F, 2.0F} + Vec2{3.0F, 4.0F} == Vec2{4.0F, 6.0F});
static_assert(dot(Vec2{1.0F, 0.0F}, Vec2{0.0F, 1.0F}) == 0.0F);

// ---------------------------------------------------------------- Rng

TEST(Rng, MatchesPcg32ReferenceOutput) {
    // First outputs of the official pcg32-demo for seed 42, sequence 54.
    Rng rng(42U, 54U);
    EXPECT_EQ(rng.next_u32(), 0xa15c02b7U);
    EXPECT_EQ(rng.next_u32(), 0x7b47f409U);
    EXPECT_EQ(rng.next_u32(), 0xba1d3330U);
    EXPECT_EQ(rng.next_u32(), 0x83d2f293U);
}

TEST(Rng, FloatsStayInUnitInterval) {
    Rng rng(7U);
    for (int i = 0; i < 100'000; ++i) {
        const float f = rng.next_float();
        ASSERT_GE(f, 0.0F);
        ASSERT_LT(f, 1.0F);
    }
}

TEST(Rng, SameSeedSameSequence) {
    Rng a(123U);
    Rng b(123U);
    for (int i = 0; i < 1000; ++i) {
        ASSERT_EQ(a.next_u32(), b.next_u32());
    }
}

// ---------------------------------------------------------------- FixedPool

TEST(FixedPool, SpawnReturnsNullWhenFull) {
    FixedPool<int, 3> pool;
    EXPECT_NE(pool.spawn(), nullptr);
    EXPECT_NE(pool.spawn(), nullptr);
    EXPECT_NE(pool.spawn(), nullptr);
    EXPECT_TRUE(pool.full());
    EXPECT_EQ(pool.spawn(), nullptr);
    EXPECT_EQ(pool.size(), 3U);
}

TEST(FixedPool, RemoveIfKeepsSurvivorsContiguous) {
    FixedPool<int, 8> pool;
    for (int i = 0; i < 8; ++i) {
        *pool.spawn() = i;
    }
    const auto removed = pool.remove_if([](int v) { return v % 2 == 0; });
    EXPECT_EQ(removed, 4U);
    ASSERT_EQ(pool.size(), 4U);

    const std::set<int> survivors(pool.begin(), pool.end());
    EXPECT_EQ(survivors, (std::set<int>{1, 3, 5, 7}));
}

TEST(FixedPool, SpawnResetsRecycledSlot) {
    FixedPool<int, 2> pool;
    *pool.spawn() = 42;
    pool.clear();
    EXPECT_EQ(*pool.spawn(), 0);
}
