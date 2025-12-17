#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>
#include <iomanip>

using namespace avs;

TEST_CASE("Color Corruption in Resampling", "[color_corruption]") {
    SECTION("16x16 black & white checkerboard with 2x2 boxes - bilinear test") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create 16x16 checkerboard with 2x2 boxes
        int width = 16, height = 16;
        std::vector<uint32_t> input(width * height);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // Create 2x2 checkerboard pattern: each 2x2 block is either all black or all white
                bool is_white = ((x / 2 + y / 2) % 2 == 0);
                input[y * width + x] = is_white ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        
        std::cout << "16x16 Checkerboard pattern (2x2 boxes):" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << (input[y * width + x] == 0xFFFFFFFF ? "W" : "B");
                if (x % 2 == 1) std::cout << " "; // Space between 2x2 boxes
            }
            std::cout << std::endl;
            if (y % 2 == 1) std::cout << std::endl; // Extra space between rows of boxes
        }
        
        std::vector<uint32_t> output(width * height, 0xFF808080);
        
        // Test identity transformation with bilinear interpolation
        // Use coarse grid to force interpolation
        table.generate(width, height, 8, 8, "x", "y", true, true, // Enable subpixel for bilinear
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "\\nAfter bilinear resampling:" << std::endl;
        
        // Analyze color corruption
        int pure_white = 0;
        int pure_black = 0;
        int corrupted = 0;
        
        std::vector<uint32_t> corrupted_colors;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t color = output[y * width + x];
                
                if (color == 0xFFFFFFFF) {
                    std::cout << "W";
                    pure_white++;
                } else if (color == 0xFF000000) {
                    std::cout << "B";
                    pure_black++;
                } else {
                    // Color corruption detected!
                    std::cout << "X";
                    corrupted++;
                    corrupted_colors.push_back(color);
                }
                if (x % 2 == 1) std::cout << " ";
            }
            std::cout << std::endl;
            if (y % 2 == 1) std::cout << std::endl;
        }
        
        std::cout << "\\nColor analysis:" << std::endl;
        std::cout << "Pure white pixels: " << pure_white << std::endl;
        std::cout << "Pure black pixels: " << pure_black << std::endl;
        std::cout << "CORRUPTED pixels: " << corrupted << std::endl;
        
        if (corrupted > 0) {
            std::cout << "\\nCorrupted colors found:" << std::endl;
            for (size_t i = 0; i < std::min(corrupted_colors.size(), (size_t)10); i++) {
                uint32_t c = corrupted_colors[i];
                uint8_t r = (c >> 16) & 0xFF;
                uint8_t g = (c >> 8) & 0xFF;
                uint8_t b = c & 0xFF;
                std::cout << "  0x" << std::hex << c << " RGB(" << std::dec 
                          << (int)r << "," << (int)g << "," << (int)b << ")" << std::endl;
            }
        }
        
        // CRITICAL: There should be NO color corruption in a black & white checkerboard
        // Any interpolation between black (0,0,0) and white (255,255,255) should only
        // produce grayscale values, never colored pixels
        REQUIRE(corrupted == 0);
    }
    
    SECTION("Test simpler pattern for color corruption") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Simple 4x4 black and white pattern
        std::vector<uint32_t> input = {
            0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFF000000,
            0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF,
            0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFF000000,
            0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF
        };
        std::vector<uint32_t> output(16, 0xFF808080);
        
        // Use identity transformation with subpixel interpolation to trigger bilinear sampling
        table.generate(4, 4, 2, 2, "x", "y", true, true, // subpixel=true for bilinear
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), 4, 4, false);
        
        std::cout << "\\n4x4 Simple test:" << std::endl;
        std::cout << "Input:  ";
        for (int i = 0; i < 4; i++) std::cout << (input[i] == 0xFFFFFFFF ? "W" : "B") << " ";
        std::cout << std::endl << "Output: ";
        for (int i = 0; i < 4; i++) {
            uint32_t c = output[i];
            if (c == 0xFFFFFFFF) std::cout << "W ";
            else if (c == 0xFF000000) std::cout << "B ";
            else {
                uint8_t r = (c >> 16) & 0xFF;
                uint8_t g = (c >> 8) & 0xFF;
                uint8_t b = c & 0xFF;
                std::cout << "C(" << (int)r << "," << (int)g << "," << (int)b << ") ";
            }
        }
        std::cout << std::endl;
        
        // Check for color corruption
        bool has_corruption = false;
        for (int i = 0; i < 16; i++) {
            uint32_t c = output[i];
            if (c != 0xFFFFFFFF && c != 0xFF000000) {
                uint8_t r = (c >> 16) & 0xFF;
                uint8_t g = (c >> 8) & 0xFF;
                uint8_t b = c & 0xFF;
                
                // Check if it's at least grayscale (R=G=B)
                if (r != g || g != b) {
                    std::cout << "COLOR CORRUPTION at pixel " << i << ": RGB(" 
                              << (int)r << "," << (int)g << "," << (int)b << ")" << std::endl;
                    has_corruption = true;
                }
            }
        }
        
        REQUIRE(!has_corruption);
    }
}