#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>
#include <iomanip>

using namespace avs;

TEST_CASE("Subpixel Sampling Artifacts", "[subpixel_artifacts]") {
    SECTION("Test subpixel sampling vs nearest neighbor") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create a 4x4 checkerboard pattern
        std::vector<uint32_t> input(16);
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                bool is_white = ((x + y) % 2 == 0);
                input[y * 4 + x] = is_white ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        
        std::cout << "4x4 Checkerboard input:" << std::endl;
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                std::cout << (input[y * 4 + x] == 0xFFFFFFFF ? "W" : "B") << " ";
            }
            std::cout << std::endl;
        }
        
        // Test both with and without subpixel sampling
        std::vector<bool> subpixel_modes = {false, true};
        std::vector<std::string> mode_names = {"Nearest", "Subpixel"};
        
        for (size_t mode = 0; mode < subpixel_modes.size(); mode++) {
            std::vector<uint32_t> output(16, 0xFF808080);
            
            std::cout << "\\n=== Testing " << mode_names[mode] << " sampling ===" << std::endl;
            
            // Generate with identity transformation but enable/disable subpixel
            table.generate(4, 4, 2, 2, "x", "y", true, subpixel_modes[mode],
                          audio_data, false, InterpolationMode::LINEAR);
            
            table.apply(input.data(), output.data(), 4, 4, false);
            
            std::cout << "Output:" << std::endl;
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    uint32_t color = output[y * 4 + x];
                    if (color == 0xFFFFFFFF) {
                        std::cout << "W ";
                    } else if (color == 0xFF000000) {
                        std::cout << "B ";
                    } else {
                        // Mixed color - show the red component as hex
                        std::cout << std::hex << ((color >> 16) & 0xFF) << std::dec << " ";
                    }
                }
                std::cout << std::endl;
            }
            
            // Check for exact matches vs interpolated colors
            int exact_matches = 0;
            int interpolated_colors = 0;
            
            for (int i = 0; i < 16; i++) {
                if (output[i] == input[i]) {
                    exact_matches++;
                } else if (output[i] != 0xFFFFFFFF && output[i] != 0xFF000000) {
                    interpolated_colors++;
                }
            }
            
            std::cout << "Exact matches: " << exact_matches << "/16" << std::endl;
            std::cout << "Interpolated colors: " << interpolated_colors << "/16" << std::endl;
            
            if (mode == 0) {
                // Nearest neighbor should have mostly exact matches
                REQUIRE(exact_matches >= 8); // At least half should be exact
            }
        }
    }
    
    SECTION("Test coordinate precision at boundaries") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Test what happens when we sample exactly at pixel boundaries
        std::vector<uint32_t> input = {
            0xFFFF0000, 0xFF00FF00,  // Red, Green
            0xFF0000FF, 0xFFFFFFFF   // Blue, White
        };
        std::vector<uint32_t> output(4);
        
        // Test sampling at exact pixel coordinates
        table.generate(2, 2, 2, 2, "x", "y", true, true,  // Enable subpixel
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), 2, 2, false);
        
        std::cout << "\\n2x2 boundary test with subpixel sampling:" << std::endl;
        for (int i = 0; i < 4; i++) {
            std::cout << "Pixel " << i << ": input=0x" << std::hex << input[i] 
                      << " output=0x" << output[i] << std::dec;
            if (input[i] != output[i]) {
                std::cout << " (CHANGED)";
            }
            std::cout << std::endl;
        }
        
        REQUIRE(true); // Just for debugging output
    }
    
    SECTION("Test bounds clamping edge case") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create a pattern where edge sampling would be problematic
        std::vector<uint32_t> input = {
            0xFFFF0000, 0xFF00FF00, 0xFF0000FF,  // Red, Green, Blue
            0xFFFFFFFF, 0xFF808080, 0xFF404040,  // White, Gray, Dark Gray
            0xFF800000, 0xFF008000, 0xFF000080   // Dark Red, Dark Green, Dark Blue
        };
        std::vector<uint32_t> output(9, 0xFF000000);
        
        // Use a transform that shifts by half a pixel to trigger edge sampling
        table.generate(3, 3, 2, 2, "x+0.001", "y+0.001", true, true,
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), 3, 3, false);
        
        std::cout << "\\n3x3 edge sampling test (shift by 0.001):" << std::endl;
        std::cout << "Input:" << std::endl;
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                std::cout << std::hex << input[y * 3 + x] << " ";
            }
            std::cout << std::endl;
        }
        
        std::cout << std::dec << "Output:" << std::endl;
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                std::cout << std::hex << output[y * 3 + x] << " ";
            }
            std::cout << std::endl;
        }
        
        // Check if colors are significantly different (indicating sampling errors)
        bool colors_similar = true;
        for (int i = 0; i < 9; i++) {
            uint32_t in_r = (input[i] >> 16) & 0xFF;
            uint32_t out_r = (output[i] >> 16) & 0xFF;
            
            if (abs((int)in_r - (int)out_r) > 10) { // Allow small differences
                std::cout << std::dec << "Large color difference at pixel " << i 
                          << ": " << in_r << " -> " << out_r << std::endl;
                colors_similar = false;
            }
        }
        
        REQUIRE(colors_similar);
    }
}