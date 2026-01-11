cmake_minimum_required(VERSION 3.15)
project(AVSTools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to avs_lib root
set(AVS_LIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Build avs_core library
add_library(avs_core STATIC
    ${AVS_LIB_ROOT}/core/effect_base.cpp
    ${AVS_LIB_ROOT}/core/parameter.cpp
    ${AVS_LIB_ROOT}/core/plugin_manager.cpp
    ${AVS_LIB_ROOT}/core/json.cpp
    ${AVS_LIB_ROOT}/core/preset.cpp
    ${AVS_LIB_ROOT}/core/builtin_effects.cpp
    ${AVS_LIB_ROOT}/core/script/script_engine.cpp
    ${AVS_LIB_ROOT}/core/script/lexer.cpp
    ${AVS_LIB_ROOT}/core/script/parser.cpp
    ${AVS_LIB_ROOT}/core/coordinate_grid.cpp
    ${AVS_LIB_ROOT}/core/blend.cpp
    ${AVS_LIB_ROOT}/effects/movement_effect.cpp
    ${AVS_LIB_ROOT}/effects/dynamic_movement_effect.cpp
    ${AVS_LIB_ROOT}/effects/brightness_effect.cpp
    ${AVS_LIB_ROOT}/effects/blur_effect.cpp
    ${AVS_LIB_ROOT}/effects/clear_effect.cpp
    ${AVS_LIB_ROOT}/effects/onbeat_clear_effect.cpp
    ${AVS_LIB_ROOT}/effects/dot_grid_effect.cpp
    ${AVS_LIB_ROOT}/effects/superscope_effect.cpp
    ${AVS_LIB_ROOT}/effects/oscilloscope_effect.cpp
    ${AVS_LIB_ROOT}/effects/color_fade_effect.cpp
    ${AVS_LIB_ROOT}/effects/ddm_effect.cpp
    ${AVS_LIB_ROOT}/effects/fadeout_effect.cpp
    ${AVS_LIB_ROOT}/effects/bump_effect.cpp
    ${AVS_LIB_ROOT}/effects/effect_list.cpp
    ${AVS_LIB_ROOT}/effects/unsupported_effect.cpp
    ${AVS_LIB_ROOT}/effects/rotoblitter_effect.cpp
    ${AVS_LIB_ROOT}/effects/custom_bpm_effect.cpp
    ${AVS_LIB_ROOT}/effects/set_render_mode_effect.cpp
    ${AVS_LIB_ROOT}/effects/mirror_effect.cpp
    ${AVS_LIB_ROOT}/effects/shift_effect.cpp
    ${AVS_LIB_ROOT}/effects/scatter_effect.cpp
    ${AVS_LIB_ROOT}/effects/dot_fountain_effect.cpp
    ${AVS_LIB_ROOT}/effects/water_effect.cpp
    ${AVS_LIB_ROOT}/effects/moving_particle_effect.cpp
    ${AVS_LIB_ROOT}/effects/interferences_effect.cpp
)

target_include_directories(avs_core PUBLIC
    ${AVS_LIB_ROOT}
    ${AVS_LIB_ROOT}/core
)

# avs2json converter
add_executable(avs2json avs2json.cpp)
target_link_libraries(avs2json PRIVATE avs_core)
target_include_directories(avs2json PRIVATE ${AVS_LIB_ROOT})

# Compiler warnings
if(MSVC)
    target_compile_options(avs_core PRIVATE /W4)
    target_compile_options(avs2json PRIVATE /W4)
else()
    target_compile_options(avs_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(avs2json PRIVATE -Wall -Wextra -pedantic)
endif()
