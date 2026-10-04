cmake_minimum_required(VERSION 3.25)

project(asteroids
    VERSION 0.1.0
    DESCRIPTION "Asteroids-style shooter with a headless, allocation-free game core"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ASTEROIDS_BUILD_GAME          "Build the SFML game (downloads SFML + Dear ImGui)" ON)
option(ASTEROIDS_BUILD_TESTS         "Build unit tests"                                   ON)
option(ASTEROIDS_BUILD_BENCH         "Build the collision benchmark"                      ON)
option(ASTEROIDS_WARNINGS_AS_ERRORS  "Treat compiler warnings as errors"                  ON)
option(ASTEROIDS_ENABLE_ASAN         "Build with AddressSanitizer"                        OFF)

# ---------------------------------------------------------------------------
# Warnings, applied only to our own targets
# ---------------------------------------------------------------------------
add_library(asteroids_warnings INTERFACE)
if(MSVC)
    target_compile_options(asteroids_warnings INTERFACE /W4 /permissive- /utf-8 /external:W0)
    if(ASTEROIDS_WARNINGS_AS_ERRORS)
        target_compile_options(asteroids_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(asteroids_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast -Wdouble-promotion)
    if(ASTEROIDS_WARNINGS_AS_ERRORS)
        target_compile_options(asteroids_warnings INTERFACE -Werror)
    endif()
endif()

if(ASTEROIDS_ENABLE_ASAN)
    if(MSVC)
        target_compile_options(asteroids_warnings INTERFACE /fsanitize=address)
    else()
        target_compile_options(asteroids_warnings INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(asteroids_warnings INTERFACE -fsanitize=address,undefined)
    endif()
endif()

# ---------------------------------------------------------------------------
# Game core: pure C++, no graphics dependency, so it builds and tests anywhere
# ---------------------------------------------------------------------------
add_library(asteroids_core src/game.cpp)
target_include_directories(asteroids_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(asteroids_core PRIVATE asteroids_warnings)

# ---------------------------------------------------------------------------
# Playable game (SFML window + Dear ImGui overlay)
# ---------------------------------------------------------------------------
if(ASTEROIDS_BUILD_GAME)
    include(cmake/Dependencies.cmake)

    add_executable(asteroids_game
        app/main.cpp
        app/renderer.cpp
        app/hud.cpp)
    target_link_libraries(asteroids_game PRIVATE
        asteroids_core asteroids_warnings SFML::Graphics ImGui-SFML::ImGui-SFML)

    # Release builds on Windows open without a console window.
    if(WIN32)
        target_link_libraries(asteroids_game PRIVATE SFML::Main)
        set_target_properties(asteroids_game PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
    endif()
endif()

if(ASTEROIDS_BUILD_BENCH)
    add_executable(collision_bench bench/collision_bench.cpp)
    target_link_libraries(collision_bench PRIVATE asteroids_core asteroids_warnings)
endif()

if(ASTEROIDS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
