#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../core/coordinate_lookup_table.h"
#include "../core/script/script_engine.h"
#include <cstring>
#include <iostream>
#include <iomanip>

using Catch::Approx;
using namespace avs;

TEST_CASE("Script Engine Direct Test", "[coordinate_debug]") {
    SECTION("Test script engine with coordinate variables") {
        ScriptEngine engine;
        
        // Test simple identity
        engine.set_variable("x", 0.5);
        engine.set_variable("y", 0.3);
        
        double result_x = engine.evaluate("x");
        double result_y = engine.evaluate("y");
        
        std::cout << "Input x=0.5, y=0.3" << std::endl;
        std::cout << "evaluate('x') = " << result_x << std::endl;
        std::cout << "evaluate('y') = " << result_y << std::endl;
        
        REQUIRE(result_x == Approx(0.5));
        REQUIRE(result_y == Approx(0.3));
        
        // Test assignment
        engine.evaluate("x=x; y=y-0.01");
        
        double new_x = engine.get_variable("x");
        double new_y = engine.get_variable("y");
        
        std::cout << "After 'x=x; y=y-0.01':" << std::endl;
        std::cout << "get_variable('x') = " << new_x << std::endl;
        std::cout << "get_variable('y') = " << new_y << std::endl;
        
        REQUIRE(new_x == Approx(0.5));
        REQUIRE(new_y == Approx(0.29));
    }
}

TEST_CASE("Coordinate Mapping Debug", "[coordinate_debug]") {
    SECTION("Test coordinate normalization") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        int width = 4, height = 4;
        int grid_width = 2, grid_height = 2;
        
        // Generate simple identity transformation
        table.generate(width, height, grid_width, grid_height,
                      "x", "y", true, false, audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0xFF000000);
        
        // Create simple test pattern
        for (int i = 0; i < width * height; i++) {
            input[i] = 0xFF000000 + (i << 16);  // Different red values
        }
        
        std::cout << "Input pattern:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << std::hex << input[y * width + x] << " ";
            }
            std::cout << std::dec << std::endl;
        }
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "Output pattern after identity:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << std::hex << output[y * width + x] << " ";
            }
            std::cout << std::dec << std::endl;
        }
        
        // For identity, they should be the same
        bool matches = true;
        for (int i = 0; i < width * height; i++) {
            if (input[i] != output[i]) {
                matches = false;
                std::cout << "Mismatch at index " << i << ": " 
                          << std::hex << input[i] << " != " << output[i] << std::dec << std::endl;
            }
        }
        
        REQUIRE(matches);
    }
}

TEST_CASE("Coordinate Space Analysis", "[coordinate_debug]") {
    SECTION("Analyze coordinate space mapping") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Test what happens to coordinate space
        int width = 8, height = 8;
        
        // Create a pattern where we know what should happen
        table.generate(width, height, width, height,  // Full resolution grid
                      "x", "y", true, false, audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height, 0xFF000000);
        std::vector<uint32_t> output(width * height, 0xFF808080);
        
        // Put white pixel in center
        input[height/2 * width + width/2] = 0xFFFFFFFF;
        
        table.apply(input.data(), output.data(), width, height, false);
        
        // Find where the white pixel ended up
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                if ((output[idx] & 0x00FFFFFF) == 0x00FFFFFF) {
                    std::cout << "White pixel found at (" << x << "," << y << ")" << std::endl;
                }
            }
        }
        
        // For identity, white pixel should still be in center
        REQUIRE((output[height/2 * width + width/2] & 0x00FFFFFF) == 0x00FFFFFF);
    }
    
    SECTION("Test coordinate calculation step by step") {
        // Let's manually calculate what should happen for a simple case
        int width = 4, height = 4;
        int grid_width = 2, grid_height = 2;
        
        std::cout << "\n=== Manual Coordinate Calculation ===" << std::endl;
        
        // For a 2x2 grid on a 4x4 image:
        // Grid points should be at:
        // (0,0) -> normalized (0.0, 0.0) 
        // (1,0) -> normalized (1.0, 0.0)
        // (0,1) -> normalized (0.0, 1.0)
        // (1,1) -> normalized (1.0, 1.0)
        
        for (int gy = 0; gy < grid_height; gy++) {
            for (int gx = 0; gx < grid_width; gx++) {
                double norm_x = (double)gx / (grid_width - 1);
                double norm_y = (double)gy / (grid_height - 1);
                
                std::cout << "Grid (" << gx << "," << gy << ") -> normalized (" 
                          << norm_x << "," << norm_y << ")" << std::endl;
            }
        }
        
        std::cout << "\nFor output pixel mapping:" << std::endl;
        for (int dest_y = 0; dest_y < height; dest_y++) {
            for (int dest_x = 0; dest_x < width; dest_x++) {
                // Calculate grid coordinates for this output pixel
                double grid_x = (dest_x * (grid_width - 1.0)) / (width - 1.0);
                double grid_y = (dest_y * (grid_height - 1.0)) / (height - 1.0);
                
                std::cout << "Output (" << dest_x << "," << dest_y << ") -> grid (" 
                          << grid_x << "," << grid_y << ")" << std::endl;
            }
        }
    }
}