cmake_minimum_required(VERSION 3.15)
project(ofxAVS VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build tests" ON)

# Core library
add_library(avs_core STATIC
    src/avs_lib/core/effect_base.h
    src/avs_lib/core/renderer.h
    src/avs_lib/core/renderer.cpp
    src/avs_lib/core/parameter.h
    src/avs_lib/core/parameter.cpp
    src/avs_lib/core/plugin_manager.h
    src/avs_lib/core/plugin_manager.cpp
)

# Set up include paths so old includes still work
target_include_directories(avs_core PUBLIC 
    src/avs_lib
    src/avs_lib/core
)

# Built-in effects library
add_library(avs_effects STATIC
    src/avs_lib/effects/clear_effect.h
    src/avs_lib/effects/clear_effect.cpp
    src/avs_lib/effects/oscilloscope_effect.h
    src/avs_lib/effects/oscilloscope_effect.cpp
    src/avs_lib/effects/blur_effect.h
    src/avs_lib/effects/blur_effect.cpp
    src/avs_lib/core/builtin_effects.h
    src/avs_lib/core/builtin_effects.cpp
)

target_link_libraries(avs_effects PUBLIC avs_core)
target_include_directories(avs_effects PUBLIC 
    src/avs_lib
    src/avs_lib/core
    src/avs_lib/effects
)

# Example application  
if(BUILD_EXAMPLES)
    add_executable(avs_example
        src/avs_lib/examples/standalone_example.cpp
    )
    
    target_link_libraries(avs_example PRIVATE avs_core avs_effects)
    target_include_directories(avs_example PRIVATE 
        src/avs_lib
        src/avs_lib/core
    )
endif()

# Install targets
install(TARGETS avs_core avs_effects
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)

install(DIRECTORY src/avs_lib/core/ 
        DESTINATION include/avs_lib/core
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