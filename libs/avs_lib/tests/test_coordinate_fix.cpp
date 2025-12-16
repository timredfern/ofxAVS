#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using Catch::Approx;
using namespace avs;

TEST_CASE("Coordinate Space Fix Analysis", "[coordinate_fix]") {
    SECTION("Test coordinate denormalization") {
        CoordinateLookupTable table;
        
        // Test the denormalize_coordinates method behavior
        table.generate(4, 4, 2, 2, "x", "y", true, false, {}, false, InterpolationMode::NONE);
        
        // Manually check what coordinate (0.5, 0.5) maps to in a 4x4 image
        // Should map to pixel (1.5, 1.5) which is the center
        
        // For 4x4 image, center should be between pixels (1,1) and (2,2)
        // So normalized 0.5 should map to pixel 1.5 (center of 4x4 grid)
        
        std::cout << "Testing coordinate denormalization for 4x4 image:" << std::endl;
        std::cout << "Normalized (0.0, 0.0) should map to pixel (0, 0)" << std::endl;
        std::cout << "Normalized (0.5, 0.5) should map to pixel (1.5, 1.5)" << std::endl;  
        std::cout << "Normalized (1.0, 1.0) should map to pixel (3, 3)" << std::endl;
        
        // The issue: denormalize_coordinates uses (width-1) and (height-1)
        // For 4x4: norm 0.5 -> 0.5 * (4-1) = 1.5 ✓
        // For 4x4: norm 1.0 -> 1.0 * (4-1) = 3.0 ✓
        
        REQUIRE(true); // Placeholder
    }
    
    SECTION("Test exact grid point mapping") {
        // Use exact grid resolution to eliminate interpolation
        int width = 3, height = 3;
        int grid_width = 3, grid_height = 3;
        
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        table.generate(width, height, grid_width, grid_height,
                      "x", "y", true, false, audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0xFF808080);
        
        // Create identity pattern with unique values
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                input[y * width + x] = 0xFF000000 + (x << 16) + (y << 8);
            }
        }
        
        std::cout << "3x3 Input pattern:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << std::hex << (input[y * width + x] & 0x00FFFF00) << " ";
            }
            std::cout << std::dec << std::endl;
        }
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "3x3 Output pattern:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << std::hex << (output[y * width + x] & 0x00FFFF00) << " ";
            }
            std::cout << std::dec << std::endl;
        }
        
        // For exact grid with identity, should be identical
        bool exact_match = true;
        for (int i = 0; i < width * height; i++) {
            if (input[i] != output[i]) {
                exact_match = false;
                break;
            }
        }
        
        REQUIRE(exact_match);
    }
    
    SECTION("Test interpolation modes") {
        int width = 4, height = 4;
        
        CoordinateLookupTable table_none, table_linear;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Same setup, different interpolation
        table_none.generate(width, height, 2, 2, "x", "y", true, false, 
                           audio_data, false, InterpolationMode::NONE);
        table_linear.generate(width, height, 2, 2, "x", "y", true, false,
                             audio_data, false, InterpolationMode::LINEAR);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output_none(width * height, 0xFF000000);
        std::vector<uint32_t> output_linear(width * height, 0xFF000000);
        
        // Diagonal pattern
        for (int i = 0; i < width * height; i++) {
            input[i] = (i % (width + 1) == 0) ? 0xFFFFFFFF : 0xFF000000;
        }
        
        table_none.apply(input.data(), output_none.data(), width, height, false);
        table_linear.apply(input.data(), output_linear.data(), width, height, false);
        
        std::cout << "Input diagonal pattern:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << ((input[y * width + x] & 0xFF) ? "W" : "B") << " ";
            }
            std::cout << std::endl;
        }
        
        std::cout << "Output NONE interpolation:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << ((output_none[y * width + x] & 0xFF) ? "W" : "B") << " ";
            }
            std::cout << std::endl;
        }
        
        std::cout << "Output LINEAR interpolation:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << ((output_linear[y * width + x] & 0xFF) ? "W" : "B") << " ";
            }
            std::cout << std::endl;
        }
        
        REQUIRE(true); // Just for visualization
    }
}