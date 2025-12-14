cmake_minimum_required(VERSION 3.15)
project(AVSExample VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the avs_lib components (assuming they're in parent directory)
set(AVS_LIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Core library
add_library(avs_core STATIC
    ${AVS_LIB_ROOT}/core/renderer.cpp
    ${AVS_LIB_ROOT}/core/parameter.cpp
    ${AVS_LIB_ROOT}/core/plugin_manager.cpp
)

target_include_directories(avs_core PUBLIC 
    ${AVS_LIB_ROOT}
    ${AVS_LIB_ROOT}/core
)

# Built-in effects library
add_library(avs_effects STATIC
    ${AVS_LIB_ROOT}/effects/clear_effect.cpp
    ${AVS_LIB_ROOT}/effects/oscilloscope_effect.cpp
    ${AVS_LIB_ROOT}/effects/blur_effect.cpp
    ${AVS_LIB_ROOT}/core/builtin_effects.cpp
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

# Compiler-specific options
if(MSVC)
    target_compile_options(avs_core PRIVATE /W4)
    target_compile_options(avs_effects PRIVATE /W4)
    target_compile_options(avs_example PRIVATE /W4)
else()
    target_compile_options(avs_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(avs_effects PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(avs_example PRIVATE -Wall -Wextra -pedantic)
endif()

# Debug/Release specific settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(avs_core PRIVATE DEBUG)
    target_compile_definitions(avs_effects PRIVATE DEBUG)
    target_compile_definitions(avs_example PRIVATE DEBUG)
endif()