cmake_minimum_required(VERSION 3.15)
project(AVSTests VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable AVS extensions
add_compile_definitions(AVS_LINE_DRAWING_EXTENSIONS)

# Fetch Catch2
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Include the avs_lib core for testing
set(AVS_LIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Build avs_core library for testing (renderer is now header-only)
add_library(avs_core_test STATIC
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
    ${AVS_LIB_ROOT}/effects/moving_particle.cpp
    ${AVS_LIB_ROOT}/effects/interferences.cpp
    ${AVS_LIB_ROOT}/effects/scatter.cpp
    ${AVS_LIB_ROOT}/effects/shift.cpp
    ${AVS_LIB_ROOT}/effects/mirror.cpp
    ${AVS_LIB_ROOT}/effects/set_render_mode.cpp
    ${AVS_LIB_ROOT}/effects/set_render_mode_ext.cpp
    ${AVS_LIB_ROOT}/effects/dot_fountain.cpp
    ${AVS_LIB_ROOT}/effects/dot_plane.cpp
    ${AVS_LIB_ROOT}/effects/water.cpp
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

target_include_directories(avs_core_test PUBLIC 
    ${AVS_LIB_ROOT}
    ${AVS_LIB_ROOT}/core
)

# Test executable
add_executable(avs_tests
    main.cpp
    test_lexer.cpp
    test_evaluator.cpp
    test_functions.cpp
    test_variables.cpp
    test_integration.cpp
    test_avs_variables.cpp
    test_audio_variables.cpp
    test_assignment_expressions.cpp
    test_coordinate_grid_new.cpp
    test_dynamic_movement_effect.cpp
    test_enum_parameter.cpp
    test_movement_presets.cpp
    test_brightness_effect.cpp
    test_complete_script_engine.cpp
    test_multi_phase_scripts.cpp
    test_semicolon_check.cpp
    test_color_format.cpp
    test_preset_binary.cpp
    test_load_parameters_colors.cpp
    test_minmax.cpp
)

target_link_libraries(avs_tests PRIVATE 
    Catch2::Catch2WithMain
    avs_core_test
)

target_include_directories(avs_tests PRIVATE
    ${AVS_LIB_ROOT}
    ${AVS_LIB_ROOT}/core
)

# Define path to test presets directory
target_compile_definitions(avs_tests PRIVATE
    TEST_PRESETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/presets"
)

# Compiler-specific options
if(MSVC)
    target_compile_options(avs_tests PRIVATE /W4)
else()
    target_compile_options(avs_tests PRIVATE -Wall -Wextra -pedantic)
endif()

# CTest integration
include(CTest)
include(Catch)
catch_discover_tests(avs_tests)

# Compatibility build test (without extensions)
# This ensures the library compiles in original AVS compatibility mode
add_library(avs_compat_test STATIC
    ${AVS_LIB_ROOT}/core/effect_base.cpp
    ${AVS_LIB_ROOT}/core/parameter.cpp
    ${AVS_LIB_ROOT}/core/blend.cpp
    ${AVS_LIB_ROOT}/effects/set_render_mode.cpp
)
target_include_directories(avs_compat_test PUBLIC
    ${AVS_LIB_ROOT}
    ${AVS_LIB_ROOT}/core
)
# Note: AVS_LINE_DRAWING_EXTENSIONS is NOT defined for this target
set_target_properties(avs_compat_test PROPERTIES
    COMPILE_DEFINITIONS ""
)

# Custom test targets for development
add_custom_target(test_lexer
    COMMAND avs_tests [lexer]
    DEPENDS avs_tests
    COMMENT "Running lexer tests"
)

add_custom_target(test_evaluator
    COMMAND avs_tests [evaluator]
    DEPENDS avs_tests
    COMMENT "Running evaluator tests"
)

