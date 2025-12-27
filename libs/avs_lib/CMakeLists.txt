cmake_minimum_required(VERSION 3.15)
project(AVSLib VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_EXAMPLES "Build example applications" ON)

# Core library
add_library(avs_core STATIC
    core/effect_base.h
    core/renderer.h
    core/parameter.h
    core/parameter.cpp
    core/plugin_manager.h
    core/plugin_manager.cpp
    core/coordinate_grid.h
    core/coordinate_grid.cpp
)

target_include_directories(avs_core PUBLIC .)

# Built-in effects library
add_library(avs_effects STATIC
    effects/clear_effect.h
    effects/clear_effect.cpp
    effects/oscilloscope_effect.h
    effects/oscilloscope_effect.cpp
    effects/blur_effect.h
    effects/blur_effect.cpp
    effects/movement_effect.h
    effects/movement_effect.cpp
    effects/dynamic_movement_effect.h
    effects/dynamic_movement_effect.cpp
    core/builtin_effects.h
    core/builtin_effects.cpp
)

target_link_libraries(avs_effects PUBLIC avs_core)
target_include_directories(avs_effects PUBLIC .)

# Example application
if(BUILD_EXAMPLES)
    add_executable(avs_example
        example/src/main.cpp
    )
    
    target_link_libraries(avs_example PRIVATE avs_core avs_effects)
    target_include_directories(avs_example PRIVATE .)
endif()

# Install targets
install(TARGETS avs_core avs_effects
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)

install(DIRECTORY core/
        DESTINATION include/avs/core
        FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY effects/
        DESTINATION include/avs/effects
        FILES_MATCHING PATTERN "*.h"
)

# Compiler-specific options
if(MSVC)
    target_compile_options(avs_core PRIVATE /W4)
    target_compile_options(avs_effects PRIVATE /W4)
else()
    target_compile_options(avs_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(avs_effects PRIVATE -Wall -Wextra -pedantic)
endif()

# Debug/Release specific settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(avs_core PRIVATE DEBUG)
    target_compile_definitions(avs_effects PRIVATE DEBUG)
endif()

# Print build configuration
message(STATUS "AVS Library Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")