#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Interpolation Bug Isolation", "[interpolation_bug]") {
    SECTION("4x4 image with 2x2 grid - interpolation issue") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // This should work if interpolation is correct
        int width = 4, height = 4;
        int grid_width = 2, grid_height = 2;
        
        table.generate(width, height, grid_width, grid_height, "x", "y", true, false,
                      audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0xFF000000);
        
        // Create simple checkerboard pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bool is_white = ((x + y) % 2 == 0);
                input[y * width + x] = is_white ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        
        std::cout << "4x4 Checkerboard input:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << (((input[y * width + x] & 0xFF) == 0xFF) ? "W" : "B") << " ";
            }
            std::cout << std::endl;
        }
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "4x4 Checkerboard output (should be identical):" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << (((output[y * width + x] & 0xFF) == 0xFF) ? "W" : "B") << " ";
            }
            std::cout << std::endl;
        }
        
        // Check if ANY pixels match correctly
        int correct_pixels = 0;
        for (int i = 0; i < width * height; i++) {
            if (input[i] == output[i]) {
                correct_pixels++;
            }
        }
        
        std::cout << "Correct pixels: " << correct_pixels << " / " << (width * height) << std::endl;
        
        // At minimum, the 4 corner pixels should be correct since they align with grid points
        REQUIRE(correct_pixels >= 4);
        
        // Ideally, for identity transformation, ALL should be correct
        bool all_correct = (correct_pixels == width * height);
        if (!all_correct) {
            std::cout << "INTERPOLATION BUG CONFIRMED: " << (width * height - correct_pixels) 
                      << " pixels are wrong" << std::endl;
        }
        
        REQUIRE(all_correct);
    }
    
    SECTION("Test with LINEAR interpolation") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        int width = 4, height = 4;
        int grid_width = 2, grid_height = 2;
        
        table.generate(width, height, grid_width, grid_height, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0xFF000000);
        
        // Solid color pattern - should be easier for interpolation
        std::fill(input.begin(), input.end(), 0xFFFF0000); // All red
        
        table.apply(input.data(), output.data(), width, height, false);
        
        int red_pixels = 0;
        for (int i = 0; i < width * height; i++) {
            if ((output[i] & 0x00FF0000) == 0x00FF0000) {
                red_pixels++;
            }
        }
        
        std::cout << "Solid red test - red pixels in output: " << red_pixels << " / " << (width * height) << std::endl;
        
        // For solid color, ALL pixels should remain red regardless of interpolation
        REQUIRE(red_pixels == width * height);
    }
}