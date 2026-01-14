cmake_minimum_required(VERSION 3.15)
project(AVSTools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable AVS extensions
add_compile_definitions(AVS_LINE_DRAWING_EXTENSIONS)

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
    ${AVS_LIB_ROOT}/effects/movement.cpp
    ${AVS_LIB_ROOT}/effects/dynamic_movement.cpp
    ${AVS_LIB_ROOT}/effects/brightness.cpp
    ${AVS_LIB_ROOT}/effects/blur.cpp
    ${AVS_LIB_ROOT}/effects/clear.cpp
    ${AVS_LIB_ROOT}/effects/onbeat_clear.cpp
    ${AVS_LIB_ROOT}/effects/dot_grid.cpp
    ${AVS_LIB_ROOT}/effects/superscope.cpp
    ${AVS_LIB_ROOT}/effects/oscilloscope.cpp
    ${AVS_LIB_ROOT}/effects/color_fade.cpp
    ${AVS_LIB_ROOT}/effects/ddm.cpp
    ${AVS_LIB_ROOT}/effects/fadeout.cpp
    ${AVS_LIB_ROOT}/effects/bump.cpp
    ${AVS_LIB_ROOT}/effects/effect_list.cpp
    ${AVS_LIB_ROOT}/effects/unsupported.cpp
    ${AVS_LIB_ROOT}/effects/rotoblitter.cpp
    ${AVS_LIB_ROOT}/effects/custom_bpm.cpp
    ${AVS_LIB_ROOT}/effects/set_render_mode.cpp
    ${AVS_LIB_ROOT}/effects/set_render_mode_ext.cpp
    ${AVS_LIB_ROOT}/effects/mirror.cpp
    ${AVS_LIB_ROOT}/effects/shift.cpp
    ${AVS_LIB_ROOT}/effects/scatter.cpp
    ${AVS_LIB_ROOT}/effects/dot_fountain.cpp
    ${AVS_LIB_ROOT}/effects/dot_plane.cpp
    ${AVS_LIB_ROOT}/effects/water.cpp
    ${AVS_LIB_ROOT}/effects/moving_particle.cpp
    ${AVS_LIB_ROOT}/effects/interferences.cpp
    ${AVS_LIB_ROOT}/effects/timescope.cpp
    ${AVS_LIB_ROOT}/effects/picture.cpp
    ${AVS_LIB_ROOT}/effects/oscstar.cpp
    ${AVS_LIB_ROOT}/effects/ring.cpp
    ${AVS_LIB_ROOT}/effects/rotstar.cpp
    ${AVS_LIB_ROOT}/effects/invert.cpp
    ${AVS_LIB_ROOT}/effects/fast_brightness.cpp
    ${AVS_LIB_ROOT}/effects/color_reduction.cpp
    ${AVS_LIB_ROOT}/effects/multiplier.cpp
    ${AVS_LIB_ROOT}/effects/channel_shift.cpp
    ${AVS_LIB_ROOT}/effects/unique_tone.cpp
    ${AVS_LIB_ROOT}/effects/grain.cpp
    ${AVS_LIB_ROOT}/effects/mosaic.cpp
    ${AVS_LIB_ROOT}/effects/interleave.cpp
    ${AVS_LIB_ROOT}/effects/starfield.cpp
    ${AVS_LIB_ROOT}/effects/bass_spin.cpp
    ${AVS_LIB_ROOT}/effects/blitter_feedback.cpp
    ${AVS_LIB_ROOT}/third_party/stb_image_impl.cpp
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
