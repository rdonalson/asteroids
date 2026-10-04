# Asteroids

![CI](https://github.com/YOUR_GITHUB_USERNAME/asteroids/actions/workflows/ci.yml/badge.svg)

A vector-graphics Asteroids shooter in modern C++20, built like a small real-time system:
a **fixed-timestep, deterministic, allocation-free** game core with no graphics dependency,
and a thin SFML + Dear ImGui front end on top.

<!-- Record a short gameplay GIF (e.g. with ScreenToGif) and put it here:
![Gameplay](docs/gameplay.gif) -->

## Controls

| Key | Action |
|---|---|
| ← → or A D | Rotate |
| ↑ or W | Thrust |
| Space | Fire |
| P | Pause |
| F1 | Debug panel |
| Enter | Restart after game over |
| Esc | Quit |

## Architecture

```
 ┌──────────────── asteroids_game (SFML + ImGui) ────────────────┐
 │ keyboard ─► Input ─┐                   Renderer ◄─ read-only  │
 │                    │  fixed 60 Hz         HUD / Debug panel   │
 └────────────────────┼───────────────────────────▲──────────────┘
                      ▼                           │
 ┌──────────────── asteroids_core (pure C++) ─────┴──────────────┐
 │ Game::step(Input)                                             │
 │   ship ─► bullets ─► asteroids ─► particles ─► collisions     │
 │   FixedPool<T, N>   SpatialGrid   PCG32 Rng    state_hash()   │
 └───────────────────────────────────────────────────────────────┘
        ▲ unit tests, determinism tests, allocation test, benchmark
```

The core never touches the keyboard, the window or the clock. It takes an `Input`
and advances exactly 1/60 s. That separation is what makes everything below testable.

## Design decisions

| Decision | Why |
|---|---|
| **Fixed timestep** with an accumulator | Physics runs at exactly 60 Hz whether the screen draws at 30 or 240 FPS, so gameplay is identical on every machine. A clamp prevents a "spiral of death" after a stall. |
| **No heap allocation during gameplay** | All objects live in `FixedPool<T, N>` (dense arrays with O(1) swap-remove). A unit test **replaces the global allocator** and runs 10 simulated minutes to prove `step()` performs zero allocations. |
| **Spatial hash grid** broad phase | Collision candidates come from the 3×3 cells around an object, not all objects. Built with a counting sort into fixed arrays every tick. A `static_assert` guarantees the cell size covers the largest collision distance. |
| **Own PCG32 random generator** | `std::uniform_real_distribution` is implementation-defined, so MSVC and GCC would generate different games from the same seed. Owning the RNG makes runs reproducible; verified against the official PCG32 reference output. |
| **Deterministic simulation** | Same seed + same inputs ⇒ bit-identical state (checked every tick via an FNV-1a `state_hash()`). The debug panel shows the seed, so any run can be reproduced. |
| **Toroidal world** | Wrap-around is handled in the math (`wrapped_delta`), in the grid (neighbour cells wrap), and in the renderer (objects on an edge are drawn on both sides). |
| **One draw call per frame** | Every line is a thin quad in a single `sf::VertexArray`, cleared but not freed each frame. |
| **Dear ImGui debug panel** | Live FPS, pool usage, grid-vs-brute-force check counts, time scale, god mode and spawn controls, as on a real game or simulation team. |

## Benchmark: grid vs brute force

`collision_bench` finds every overlapping pair among N random circles both ways and fails
if the results differ. Release build, GCC 13, Linux:

| Objects | Brute force | Spatial grid | Speedup |
|---:|---:|---:|---:|
| 500 | 0.58 ms | 0.08 ms | 6.9× |
| 1,000 | 2.57 ms | 0.26 ms | 10.1× |
| 2,000 | 10.93 ms | 0.73 ms | 15.0× |
| 4,000 | 45.09 ms | 2.19 ms | 20.6× |
| 8,000 | 215.28 ms | 9.93 ms | 21.7× |

Brute force grows as O(n²); the grid stays close to linear.

## Building

The build requires CMake 3.25+ and a C++20 compiler (Visual Studio 2022+, GCC 13+ or Clang 17+).
SFML 3.0.2, Dear ImGui 1.91.9b and ImGui-SFML 3.0 are downloaded and built automatically
the first time you configure, which takes a few minutes.

### Visual Studio
1. Install the **Desktop development with C++** workload.
2. *File → Open → Folder…* and select this folder.
3. Pick the **Windows MSVC Debug** preset, then *Build → Build All*.
4. Select **asteroids_game.exe** as the startup item and press F5.
5. *Test → Test Explorer → Run All* runs the unit tests.

### VS Code
Open the folder from **Developer PowerShell for VS** (`code .`), install the recommended
extensions, choose the **Windows MSVC Debug** preset, then *CMake: Build* / *CMake: Run Tests*.

### Linux / WSL
```bash
sudo apt install ninja-build libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
                 libgl1-mesa-dev libfreetype-dev
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
./build/linux-gcc-debug/asteroids_game
```
To build only the core, tests and benchmark (no graphics libraries needed), use
`linux-gcc-release-core`. `linux-clang-asan` runs the tests under AddressSanitizer
and UndefinedBehaviorSanitizer.

## Quality checks

Every push runs these through GitHub Actions:

- Full game + tests on **Windows (MSVC)** and **Linux (GCC)**
- Core + tests under **AddressSanitizer + UndefinedBehaviorSanitizer** (Clang)
- Collision benchmark (fails if grid and brute force ever disagree)
- All compiler warnings treated as errors (`/W4 /WX`, `-Wall -Wextra -Wconversion -Werror` …)
- `clang-format` style check and `clang-tidy` static analysis

## Project layout

```
include/asteroids/  game core headers: config, vec2, rng, fixed_pool, spatial_grid, game
src/                game core implementation
app/                SFML window, fixed-timestep loop, renderer, HUD, debug panel
tests/              GoogleTest unit, determinism and no-allocation tests
bench/              collision benchmark
cmake/              third-party dependencies (FetchContent)
```

## Roadmap

- [ ] Flying saucers that shoot back (aimed with lead prediction)
- [ ] Input recording and replay files (the deterministic core already supports this)
- [ ] Sound effects (SFML audio module)
- [ ] Render interpolation between simulation ticks for smoother motion on high-refresh displays
- [ ] High-score table saved to disk
