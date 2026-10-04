#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace asteroids {

/// Fixed-capacity, densely packed object pool.
///
/// Live objects are always contiguous in [0, size()), so iterating them is a
/// straight walk through memory with no "is alive?" checks. Removal swaps the
/// last element into the hole (O(1)), which changes order but never allocates.
template <typename T, std::size_t Capacity>
class FixedPool {
public:
    /// Adds a default-constructed object and returns it, or nullptr if full.
    /// The pointer stays valid until the next removal.
    [[nodiscard]] T* spawn() noexcept {
        if (size_ == Capacity) {
            return nullptr;
        }
        items_[size_] = T{};
        return &items_[size_++];
    }

    /// Removes every object matching pred. Returns the number removed.
    template <typename Pred>
    std::size_t remove_if(Pred pred) noexcept {
        std::size_t removed = 0;
        std::size_t i = 0;
        while (i < size_) {
            if (pred(items_[i])) {
                items_[i] = items_[size_ - 1];
                --size_;
                ++removed;
            } else {
                ++i;
            }
        }
        return removed;
    }

    void clear() noexcept { size_ = 0; }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] bool full() const noexcept { return size_ == Capacity; }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

    [[nodiscard]] T& operator[](std::size_t i) noexcept { return items_[i]; }
    [[nodiscard]] const T& operator[](std::size_t i) const noexcept { return items_[i]; }

    [[nodiscard]] std::span<T> items() noexcept { return {items_.data(), size_}; }
    [[nodiscard]] std::span<const T> items() const noexcept { return {items_.data(), size_}; }

    [[nodiscard]] auto begin() noexcept { return items().begin(); }
    [[nodiscard]] auto end() noexcept { return items().end(); }
    [[nodiscard]] auto begin() const noexcept { return items().begin(); }
    [[nodiscard]] auto end() const noexcept { return items().end(); }

private:
    std::array<T, Capacity> items_{};
    std::size_t size_{0};
};

}  // namespace asteroids
