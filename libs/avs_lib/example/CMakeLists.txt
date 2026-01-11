cmake_minimum_required(VERSION 3.15)
project(AVSExample VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(AVS_LIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Core library
add_library(avs_core STATIC
    ${AVS_LIB_ROOT}/core/effect_base.cpp
    ${AVS_LIB_ROOT}/core/parameter.cpp
    ${AVS_LIB_ROOT}/core/plugin_manager.cpp
    ${AVS_LIB_ROOT}/core/json.cpp
    ${AVS_LIB_ROOT}/core/preset.cpp
    ${AVS_LIB_ROOT}/core/script/script_engine.cpp
    ${AVS_LIB_ROOT}/core/script/lexer.cpp
    ${AVS_LIB_ROOT}/core/script/parser.cpp
    ${AVS_LIB_ROOT}/core/coordinate_grid.cpp
    ${AVS_LIB_ROOT}/core/blend.cpp
)

target_include_directories(avs_core PUBLIC
    ${AVS_LIB_ROOT}
    ${AVS_LIB_ROOT}/core
)

# Built-in effects library
add_library(avs_effects STATIC
    ${AVS_LIB_ROOT}/core/builtin_effects.cpp
    ${AVS_LIB_ROOT}/effects/clear_effect.cpp
    ${AVS_LIB_ROOT}/effects/onbeat_clear_effect.cpp
    ${AVS_LIB_ROOT}/effects/oscilloscope_effect.cpp
    ${AVS_LIB_ROOT}/effects/superscope_effect.cpp
    ${AVS_LIB_ROOT}/effects/blur_effect.cpp
    ${AVS_LIB_ROOT}/effects/brightness_effect.cpp
    ${AVS_LIB_ROOT}/effects/movement_effect.cpp
    ${AVS_LIB_ROOT}/effects/dynamic_movement_effect.cpp
    ${AVS_LIB_ROOT}/effects/dot_grid_effect.cpp
    ${AVS_LIB_ROOT}/effects/color_fade_effect.cpp
    ${AVS_LIB_ROOT}/effects/ddm_effect.cpp
    ${AVS_LIB_ROOT}/effects/fadeout_effect.cpp
    ${AVS_LIB_ROOT}/effects/bump_effect.cpp
    ${AVS_LIB_ROOT}/effects/effect_list.cpp
    ${AVS_LIB_ROOT}/effects/unsupported_effect.cpp
)

target_link_libraries(avs_effects PUBLIC avs_core)
target_include_directories(avs_effects PUBLIC
    ${AVS_LIB_ROOT}
    ${AVS_LIB_ROOT}/core
    ${AVS_LIB_ROOT}/effects
)

# Example application
add_executable(avs_example
    src/main.cpp
)

target_link_libraries(avs_example PRIVATE avs_core avs_effects)
target_include_directories(avs_example PRIVATE
    ${AVS_LIB_ROOT}
    ${AVS_LIB_ROOT}/core
)

# Compiler warnings
if(MSVC)
    target_compile_options(avs_core PRIVATE /W4)
    target_compile_options(avs_effects PRIVATE /W4)
    target_compile_options(avs_example PRIVATE /W4)
else()
    target_compile_options(avs_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(avs_effects PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(avs_example PRIVATE -Wall -Wextra -pedantic)
endif()
