#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "asteroids/config.hpp"
#include "asteroids/vec2.hpp"

namespace asteroids {

/// Uniform-grid broad phase for collision detection on a wrapping world.
///
/// Instead of testing every object against every other object (O(n^2)), objects
/// are bucketed into cells and a query only looks at the 3x3 cells around a point.
/// Built with a counting sort into fixed arrays: no allocation, and rebuilt from
/// scratch every tick in O(n).
template <std::size_t CellsX, std::size_t CellsY, std::size_t MaxItems>
class SpatialGrid {
    static_assert(CellsX >= 3 && CellsY >= 3, "3x3 neighbourhood needs at least 3 cells per axis");
    static_assert(MaxItems <= 0xFFFF, "indices are stored as uint16_t");

public:
    static constexpr float kCellWidth = config::kWorldWidth / static_cast<float>(CellsX);
    static constexpr float kCellHeight = config::kWorldHeight / static_cast<float>(CellsY);

    /// Rebuild from `count` items; get_pos(i) returns the position of item i.
    template <typename GetPos>
    void build(std::size_t count, GetPos get_pos) noexcept {
        count_ = std::min(count, MaxItems);
        cell_start_.fill(0);

        for (std::size_t i = 0; i < count_; ++i) {
            const auto cell = cell_index(get_pos(i));
            item_cell_[i] = static_cast<std::uint16_t>(cell);
            ++cell_start_[cell + 1];
        }
        for (std::size_t c = 0; c < kCellCount; ++c) {
            cell_start_[c + 1] = static_cast<std::uint16_t>(cell_start_[c + 1] + cell_start_[c]);
        }
        std::array<std::uint16_t, kCellCount> cursor{};
        std::copy_n(cell_start_.begin(), kCellCount, cursor.begin());
        for (std::size_t i = 0; i < count_; ++i) {
            items_[cursor[item_cell_[i]]++] = static_cast<std::uint16_t>(i);
        }
    }

    /// Calls fn(index) for every item in the 3x3 cells around p (with wrap-around).
    /// These are candidates only: the caller still does the exact circle test.
    template <typename Fn>
    void for_each_near(Vec2 p, Fn fn) const {
        const auto [cx, cy] = cell_coords(p);
        for (std::size_t dy = 0; dy < 3; ++dy) {
            const std::size_t y = (cy + CellsY + dy - 1) % CellsY;
            for (std::size_t dx = 0; dx < 3; ++dx) {
                const std::size_t x = (cx + CellsX + dx - 1) % CellsX;
                const std::size_t cell = y * CellsX + x;
                for (std::size_t k = cell_start_[cell]; k < cell_start_[cell + 1]; ++k) {
                    fn(static_cast<std::size_t>(items_[k]));
                }
            }
        }
    }

    [[nodiscard]] std::size_t size() const noexcept { return count_; }

private:
    static constexpr std::size_t kCellCount = CellsX * CellsY;

    struct Coords {
        std::size_t x;
        std::size_t y;
    };

    [[nodiscard]] static Coords cell_coords(Vec2 p) noexcept {
        const Vec2 w = wrap_position(p);
        const auto x = std::min(static_cast<std::size_t>(w.x / kCellWidth), CellsX - 1);
        const auto y = std::min(static_cast<std::size_t>(w.y / kCellHeight), CellsY - 1);
        return {x, y};
    }

    [[nodiscard]] static std::size_t cell_index(Vec2 p) noexcept {
        const auto [x, y] = cell_coords(p);
        return y * CellsX + x;
    }

    std::array<std::uint16_t, kCellCount + 1> cell_start_{};
    std::array<std::uint16_t, MaxItems> items_{};
    std::array<std::uint16_t, MaxItems> item_cell_{};
    std::size_t count_{0};
};

}  // namespace asteroids
