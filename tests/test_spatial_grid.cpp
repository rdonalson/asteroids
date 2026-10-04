#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "asteroids/rng.hpp"
#include "asteroids/spatial_grid.hpp"

using namespace asteroids;

namespace {

using Grid = SpatialGrid<16, 9, 1000>;

std::vector<Vec2> random_points(std::size_t n, std::uint64_t seed) {
    Rng rng(seed);
    std::vector<Vec2> pts(n);
    for (auto& p : pts) {
        p = {rng.range(0.0F, config::kWorldWidth), rng.range(0.0F, config::kWorldHeight)};
    }
    return pts;
}

}  // namespace

// The grid is only an optimisation, so it must never miss a pair that the
// brute-force search would find. Checked for many random queries.
TEST(SpatialGrid, NeverMissesANeighbourWithinOneCell) {
    const auto pts = random_points(1000, 1);
    Grid grid;
    grid.build(pts.size(), [&](std::size_t i) { return pts[i]; });

    const float reach = std::min(Grid::kCellWidth, Grid::kCellHeight);
    const auto queries = random_points(500, 2);
    for (const Vec2 q : queries) {
        std::vector<std::size_t> candidates;
        grid.for_each_near(q, [&](std::size_t i) { candidates.push_back(i); });

        for (std::size_t i = 0; i < pts.size(); ++i) {
            if (circles_overlap(q, reach, pts[i], 0.0F)) {
                ASSERT_NE(std::find(candidates.begin(), candidates.end(), i), candidates.end())
                    << "grid missed point " << i;
            }
        }
    }
}

TEST(SpatialGrid, FindsNeighbourAcrossWorldEdge) {
    const std::vector<Vec2> pts{{config::kWorldWidth - 3.0F, 100.0F}};
    Grid grid;
    grid.build(pts.size(), [&](std::size_t i) { return pts[i]; });

    bool found = false;
    grid.for_each_near({3.0F, 100.0F}, [&](std::size_t) { found = true; });
    EXPECT_TRUE(found);
}

TEST(SpatialGrid, VisitsEachItemOnceAndFarFewerThanAll) {
    const auto pts = random_points(1000, 3);
    Grid grid;
    grid.build(pts.size(), [&](std::size_t i) { return pts[i]; });

    std::vector<int> seen(pts.size(), 0);
    std::size_t visited = 0;
    grid.for_each_near({640.0F, 360.0F}, [&](std::size_t i) {
        ++seen[i];
        ++visited;
    });
    EXPECT_TRUE(std::all_of(seen.begin(), seen.end(), [](int s) { return s <= 1; }));
    EXPECT_LT(visited, pts.size() / 5);  // 9 of 144 cells, roughly 6%
}
