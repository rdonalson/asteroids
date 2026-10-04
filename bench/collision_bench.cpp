// Compares brute-force all-pairs collision detection against the spatial grid.
// Both must find exactly the same overlapping pairs; the grid should be faster.

#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <vector>

#include "asteroids/rng.hpp"
#include "asteroids/spatial_grid.hpp"

using namespace asteroids;

namespace {

constexpr float kRadius = 12.0F;
constexpr std::size_t kMaxObjects = 8000;
using Grid = SpatialGrid<config::kGridCellsX, config::kGridCellsY, kMaxObjects>;

std::uint64_t brute_force(const std::vector<Vec2>& pts) {
    std::uint64_t pairs = 0;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        for (std::size_t j = i + 1; j < pts.size(); ++j) {
            pairs += circles_overlap(pts[i], kRadius, pts[j], kRadius) ? 1U : 0U;
        }
    }
    return pairs;
}

std::uint64_t with_grid(const std::vector<Vec2>& pts, Grid& grid) {
    grid.build(pts.size(), [&](std::size_t i) { return pts[i]; });
    std::uint64_t pairs = 0;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        grid.for_each_near(pts[i], [&](std::size_t j) {
            if (j > i && circles_overlap(pts[i], kRadius, pts[j], kRadius)) {
                ++pairs;
            }
        });
    }
    return pairs;
}

template <typename Fn>
double time_ms(int reps, Fn fn, std::uint64_t& result) {
    const auto start = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r) {
        result = fn();
    }
    const std::chrono::duration<double, std::milli> elapsed =
        std::chrono::steady_clock::now() - start;
    return elapsed.count() / reps;
}

}  // namespace

int main() {
    auto grid = std::make_unique<Grid>();
    std::cout << std::format("{:>8} {:>14} {:>14} {:>9} {:>8}\n", "objects", "brute (ms)",
                             "grid (ms)", "speedup", "pairs");

    bool all_match = true;
    for (const std::size_t n : {100U, 500U, 1000U, 2000U, 4000U, 8000U}) {
        Rng rng(n);
        std::vector<Vec2> pts(n);
        for (auto& p : pts) {
            p = {rng.range(0.0F, config::kWorldWidth), rng.range(0.0F, config::kWorldHeight)};
        }

        const int reps = n <= 1000 ? 50 : 5;
        std::uint64_t brute_pairs = 0;
        std::uint64_t grid_pairs = 0;
        const double brute_ms = time_ms(reps, [&] { return brute_force(pts); }, brute_pairs);
        const double grid_ms = time_ms(reps, [&] { return with_grid(pts, *grid); }, grid_pairs);

        all_match = all_match && brute_pairs == grid_pairs;
        std::cout << std::format("{:>8} {:>14.3f} {:>14.3f} {:>8.1f}x {:>8}{}\n", n, brute_ms,
                                 grid_ms, brute_ms / grid_ms, grid_pairs,
                                 brute_pairs == grid_pairs ? "" : "  MISMATCH");
    }
    return all_match ? 0 : 1;
}
