#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Movement Artifacts Test", "[movement_artifacts]") {
    SECTION("Test small movement with color preservation") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create a distinct pattern that would show artifacts
        std::vector<uint32_t> input = {
            0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFFFF,  // Red, Green, Blue, White
            0xFF800000, 0xFF008000, 0xFF000080, 0xFF808080,  // Dark red, green, blue, gray
            0xFF400000, 0xFF004000, 0xFF000040, 0xFF404040,  // Darker variants
            0xFF000000, 0xFF200020, 0xFF002000, 0xFF200000   // Black and dark variants
        };
        std::vector<uint32_t> output(16, 0xFF000000);
        
        std::cout << "Testing small downward movement (y=y-0.01):" << std::endl;
        
        // Test a small movement that should preserve most colors
        table.generate(4, 4, 4, 4, "x", "y-0.01", true, true,  // Small downward movement with subpixel
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), 4, 4, false);
        
        std::cout << "Input pattern:" << std::endl;
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                std::cout << std::hex << input[y * 4 + x] << " ";
            }
            std::cout << std::endl;
        }
        
        std::cout << std::dec << "\\nOutput after y-0.01 movement:" << std::endl;
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                std::cout << std::hex << output[y * 4 + x] << " ";
            }
            std::cout << std::endl;
        }
        
        // Check that colors haven't drastically changed
        // With a 0.01 movement on a 4x4 image, most pixels should be very similar
        int dramatically_changed = 0;
        for (int i = 0; i < 16; i++) {
            uint32_t in_r = (input[i] >> 16) & 0xFF;
            uint32_t in_g = (input[i] >> 8) & 0xFF;
            uint32_t in_b = input[i] & 0xFF;
            
            uint32_t out_r = (output[i] >> 16) & 0xFF;
            uint32_t out_g = (output[i] >> 8) & 0xFF;
            uint32_t out_b = output[i] & 0xFF;
            
            int r_diff = abs((int)in_r - (int)out_r);
            int g_diff = abs((int)in_g - (int)out_g);
            int b_diff = abs((int)in_b - (int)out_b);
            
            if (r_diff > 50 || g_diff > 50 || b_diff > 50) {
                dramatically_changed++;
                std::cout << std::dec << "Large change at pixel " << i << ": RGB(" 
                          << in_r << "," << in_g << "," << in_b << ") -> (" 
                          << out_r << "," << out_g << "," << out_b << ")" << std::endl;
            }
        }
        
        std::cout << "Dramatically changed pixels: " << dramatically_changed << "/16" << std::endl;
        
        // With small movement, most colors should be preserved
        REQUIRE(dramatically_changed <= 4); // Allow some interpolation at edges
    }
    
    SECTION("Test identity transformation preserves all colors exactly") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Use a gradient pattern to test precision
        std::vector<uint32_t> input(16);
        for (int i = 0; i < 16; i++) {
            uint8_t val = i * 16; // Create gradient from 0 to 240
            input[i] = 0xFF000000 | (val << 16) | (val << 8) | val; // Grayscale gradient
        }
        std::vector<uint32_t> output(16, 0xFF000000);
        
        // Test perfect identity
        table.generate(4, 4, 4, 4, "x", "y", true, true,
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), 4, 4, false);
        
        std::cout << "\\nIdentity transformation test:" << std::endl;
        
        // With exact grid and identity transform, should be pixel-perfect
        int exact_matches = 0;
        for (int i = 0; i < 16; i++) {
            if (input[i] == output[i]) {
                exact_matches++;
            } else {
                std::cout << "Pixel " << i << " changed: 0x" << std::hex 
                          << input[i] << " -> 0x" << output[i] << std::dec << std::endl;
            }
        }
        
        std::cout << "Exact matches: " << exact_matches << "/16" << std::endl;
        
        REQUIRE(exact_matches == 16); // Should be perfect for exact grid identity
    }
}