# Prefer an installed GoogleTest (vcpkg, apt, etc.); otherwise download it.
find_package(GTest 1.12 QUIET)

if(NOT GTest_FOUND)
    message(STATUS "GoogleTest not found locally - fetching it")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
        GIT_SHALLOW    TRUE
        SYSTEM)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(asteroids_tests
    test_math.cpp
    test_spatial_grid.cpp
    test_game.cpp)
target_link_libraries(asteroids_tests PRIVATE asteroids_core asteroids_warnings GTest::gtest_main)

# Separate binary: it replaces the global allocator, which must not affect other tests.
add_executable(asteroids_alloc_tests test_no_allocation.cpp)
target_link_libraries(asteroids_alloc_tests PRIVATE asteroids_core asteroids_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(asteroids_tests)
gtest_discover_tests(asteroids_alloc_tests)
