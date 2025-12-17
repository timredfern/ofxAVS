#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Oscilloscope Color Corruption", "[oscilloscope_corruption]") {
    SECTION("Test oscilloscope-like pattern with movement") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create oscilloscope-like pattern: mostly black with white pixels scattered
        int width = 32, height = 32;
        std::vector<uint32_t> input(width * height, 0xFF000000); // Black background
        
        // Draw a white line across the middle (like oscilloscope trace)
        int mid_y = height / 2;
        for (int x = 0; x < width; x += 2) { // Sparse white pixels
            input[mid_y * width + x] = 0xFFFFFFFF; // Pure white
        }
        
        // Add a few isolated white pixels  
        input[5 * width + 5] = 0xFFFFFFFF;
        input[20 * width + 25] = 0xFFFFFFFF;
        input[15 * width + 10] = 0xFFFFFFFF;
        
        std::cout << "Oscilloscope test pattern (32x32 with white trace):" << std::endl;
        std::cout << "White pixels at: ";
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (input[y * width + x] == 0xFFFFFFFF) {
                    std::cout << "(" << x << "," << y << ") ";
                }
            }
        }
        std::cout << std::endl;
        
        // Test with movement that mimics ofxAVS usage
        std::vector<uint32_t> output(width * height, 0xFF000000);
        
        // Use similar settings to ofxAVS: 16x16 grid on 32x32 image with small movement
        table.generate(width, height, 16, 16, "x", "y-0.01", true, false, // No subpixel initially
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "\\nAfter small movement (y-0.01) with 16x16 grid:" << std::endl;
        
        // Analyze what happened to the white pixels
        int white_in = 0, white_out = 0, colored_out = 0, black_out = 0;
        std::vector<uint32_t> colored_pixels;
        
        for (int i = 0; i < width * height; i++) {
            if (input[i] == 0xFFFFFFFF) white_in++;
            
            uint32_t out_color = output[i];
            if (out_color == 0xFFFFFFFF) {
                white_out++;
            } else if (out_color == 0xFF000000) {
                black_out++;
            } else {
                colored_out++;
                colored_pixels.push_back(out_color);
                
                // Show position of colored artifact
                int x = i % width;
                int y = i / width;
                uint8_t r = (out_color >> 16) & 0xFF;
                uint8_t g = (out_color >> 8) & 0xFF;
                uint8_t b = out_color & 0xFF;
                std::cout << "COLORED PIXEL at (" << x << "," << y << "): RGB(" 
                          << (int)r << "," << (int)g << "," << (int)b << ")" << std::endl;
            }
        }
        
        std::cout << "Input: " << white_in << " white pixels" << std::endl;
        std::cout << "Output: " << white_out << " white, " << black_out << " black, " 
                  << colored_out << " COLORED" << std::endl;
        
        // There should be NO colored pixels from resampling pure white on black
        REQUIRE(colored_out == 0);
    }
    
    SECTION("Test with subpixel sampling enabled") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Simple white dot on black background
        std::vector<uint32_t> input(16, 0xFF000000);
        input[5] = 0xFFFFFFFF; // One white pixel
        input[10] = 0xFFFFFFFF; // Another white pixel
        
        std::vector<uint32_t> output(16, 0xFF000000);
        
        std::cout << "\\nTesting subpixel sampling with white dots:" << std::endl;
        
        // Test with subpixel sampling enabled (this is what might cause issues)
        table.generate(4, 4, 2, 2, "x", "y", true, true, // subpixel=true
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), 4, 4, false);
        
        std::cout << "Input white pixels at positions: ";
        for (int i = 0; i < 16; i++) {
            if (input[i] == 0xFFFFFFFF) std::cout << i << " ";
        }
        std::cout << std::endl;
        
        std::cout << "Output analysis:" << std::endl;
        bool found_corruption = false;
        for (int i = 0; i < 16; i++) {
            uint32_t c = output[i];
            if (c != 0xFF000000 && c != 0xFFFFFFFF) {
                uint8_t r = (c >> 16) & 0xFF;
                uint8_t g = (c >> 8) & 0xFF; 
                uint8_t b = c & 0xFF;
                std::cout << "  Position " << i << ": RGB(" << (int)r << "," << (int)g << "," << (int)b << ")";
                
                if (r != g || g != b) {
                    std::cout << " CORRUPTED";
                    found_corruption = true;
                } else {
                    std::cout << " (gray - OK)";
                }
                std::cout << std::endl;
            }
        }
        
        REQUIRE(!found_corruption);
    }
}