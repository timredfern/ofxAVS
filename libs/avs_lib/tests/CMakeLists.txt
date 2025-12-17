cmake_minimum_required(VERSION 3.15)
project(AVSTests VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    ${AVS_LIB_ROOT}/core/parameter.cpp
    ${AVS_LIB_ROOT}/core/plugin_manager.cpp
    ${AVS_LIB_ROOT}/core/script/script_engine.cpp
    ${AVS_LIB_ROOT}/core/script/lexer.cpp
    ${AVS_LIB_ROOT}/core/script/parser.cpp
    ${AVS_LIB_ROOT}/core/coordinate_lookup_table.cpp
    ${AVS_LIB_ROOT}/effects/movement_effect.cpp
    ${AVS_LIB_ROOT}/effects/dynamic_movement_effect.cpp
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
    test_coordinate_lookup_table.cpp
    test_coordinate_interpolation.cpp
    test_coordinate_grid.cpp
    test_coordinate_debug.cpp
    test_coordinate_fix.cpp
    test_identity_only.cpp
    test_interpolation_bug.cpp
    test_grid_calculation.cpp
    test_interpolation_fix.cpp
    test_coordinate_precision.cpp
    test_grid_debug.cpp
    test_debug_detailed.cpp
    test_grid_population.cpp
    test_script_trace.cpp
    test_pixel_sampling.cpp
    test_subpixel_artifacts.cpp
    test_movement_artifacts.cpp
    test_dynamic_movement_realistic.cpp
    test_color_corruption.cpp
    test_oscilloscope_corruption.cpp
    test_exact_ofxavs_scenario.cpp
    test_dynamic_movement_effect.cpp
    test_enum_parameter.cpp
    test_movement_presets.cpp
    test_sample_pixel_bug.cpp
    test_pipeline_trace.cpp
    test_bilinear_bug.cpp
    test_interpolate_pixels_precision.cpp
    test_subpixel_bug.cpp
)

target_link_libraries(avs_tests PRIVATE 
    Catch2::Catch2WithMain
    avs_core_test
)

target_include_directories(avs_tests PRIVATE 
    ${AVS_LIB_ROOT}
    ${AVS_LIB_ROOT}/core
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