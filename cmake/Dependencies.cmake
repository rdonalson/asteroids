# Downloads and builds SFML, Dear ImGui and ImGui-SFML from source.
# SYSTEM marks their headers as third-party so our strict warnings don't apply to them.

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)  # static: no DLLs to copy next to the .exe
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
    GIT_SHALLOW    TRUE
    SYSTEM)

# ImGui-SFML v3.0 predates Dear ImGui 1.92's font rewrite, so pin the last 1.91 release.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9b
    GIT_SHALLOW    TRUE
    SYSTEM)

FetchContent_MakeAvailable(SFML imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_SFML_IMGUI_DEMO OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v3.0
    GIT_SHALLOW    TRUE
    SYSTEM)

FetchContent_MakeAvailable(ImGui-SFML)
