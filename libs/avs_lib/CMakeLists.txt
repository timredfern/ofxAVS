cmake_minimum_required(VERSION 3.15)
project(AVSLib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES "Build example applications" ON)

# Core library
add_library(avs_core STATIC
    core/effect_base.cpp
    core/parameter.cpp
    core/plugin_manager.cpp
    core/json.cpp
    core/preset.cpp
    core/script/script_engine.cpp
    core/script/lexer.cpp
    core/script/parser.cpp
    core/coordinate_grid.cpp
    core/blend.cpp
)

target_include_directories(avs_core PUBLIC . core)

# Built-in effects library
add_library(avs_effects STATIC
    core/builtin_effects.cpp
    effects/clear_effect.cpp
    effects/onbeat_clear_effect.cpp
    effects/oscilloscope_effect.cpp
    effects/superscope_effect.cpp
    effects/blur_effect.cpp
    effects/brightness_effect.cpp
    effects/movement_effect.cpp
    effects/dynamic_movement_effect.cpp
    effects/dot_grid_effect.cpp
    effects/color_fade_effect.cpp
    effects/ddm_effect.cpp
    effects/fadeout_effect.cpp
    effects/bump_effect.cpp
    effects/effect_list.cpp
    effects/unsupported_effect.cpp
)

target_link_libraries(avs_effects PUBLIC avs_core)
target_include_directories(avs_effects PUBLIC . core effects)

# Example application
if(BUILD_EXAMPLES)
    add_executable(avs_example
        example/src/main.cpp
    )
    target_link_libraries(avs_example PRIVATE avs_core avs_effects)
    target_include_directories(avs_example PRIVATE .)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(avs_core PRIVATE /W4)
    target_compile_options(avs_effects PRIVATE /W4)
else()
    target_compile_options(avs_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(avs_effects PRIVATE -Wall -Wextra -pedantic)
endif()

message(STATUS "AVS Library Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
