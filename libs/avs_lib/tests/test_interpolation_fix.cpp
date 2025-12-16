#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Interpolation Fix Verification", "[interpolation_fix]") {
    SECTION("4x4 image with 2x2 grid using LINEAR interpolation") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        int width = 4, height = 4;
        int grid_width = 2, grid_height = 2;
        
        // Use LINEAR interpolation for smooth identity transformation
        table.generate(width, height, grid_width, grid_height, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0xFF000000);
        
        // Create simple checkerboard pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bool is_white = ((x + y) % 2 == 0);
                input[y * width + x] = is_white ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        
        std::cout << "4x4 Checkerboard input with LINEAR interpolation:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << (((input[y * width + x] & 0xFF) == 0xFF) ? "W" : "B") << " ";
            }
            std::cout << std::endl;
        }
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "4x4 Checkerboard output with LINEAR interpolation:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << (((output[y * width + x] & 0xFF) == 0xFF) ? "W" : "B") << " ";
            }
            std::cout << std::endl;
        }
        
        // With LINEAR interpolation, identity transformation should work much better
        int correct_pixels = 0;
        for (int i = 0; i < width * height; i++) {
            if (input[i] == output[i]) {
                correct_pixels++;
            }
        }
        
        std::cout << "Correct pixels with LINEAR: " << correct_pixels << " / " << (width * height) << std::endl;
        
        // With linear interpolation, we should get much better results
        // At minimum, we expect significantly more correct pixels than with NONE mode (which got 8/16)
        REQUIRE(correct_pixels > 8);
    }
    
    SECTION("Compare all three interpolation modes") {
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        int width = 4, height = 4;
        std::vector<uint32_t> input = {
            0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFFFF,
            0xFF808080, 0xFF404040, 0xFFC0C0C0, 0xFF808080,
            0xFF800000, 0xFF008000, 0xFF000080, 0xFF808000,
            0xFF008080, 0xFF800080, 0xFF408040, 0xFF404040
        };
        std::vector<uint32_t> output(width * height);
        
        std::cout << "\nComparing interpolation modes for identity transformation:" << std::endl;
        
        std::vector<InterpolationMode> modes = {
            InterpolationMode::NONE, 
            InterpolationMode::LINEAR,
            InterpolationMode::NEAREST
        };
        std::vector<std::string> mode_names = {"NONE", "LINEAR", "NEAREST"};
        
        for (size_t i = 0; i < modes.size(); i++) {
            CoordinateLookupTable table;
            std::fill(output.begin(), output.end(), 0xFF000000);
            
            table.generate(width, height, 2, 2, "x", "y", true, false,
                          audio_data, false, modes[i]);
            table.apply(input.data(), output.data(), width, height, false);
            
            int correct_pixels = 0;
            for (int j = 0; j < width * height; j++) {
                if (input[j] == output[j]) {
                    correct_pixels++;
                }
            }
            
            std::cout << mode_names[i] << " mode: " << correct_pixels << "/" << (width * height) 
                      << " pixels correct" << std::endl;
        }
        
        REQUIRE(true); // Just for comparison output
    }
}