#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>
#include <iomanip>

using namespace avs;

TEST_CASE("Bilinear Interpolation Bug", "[bilinear_bug]") {
    SECTION("Test bilinear interpolation with subpixel=true to find color corruption") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create simple 4x4 black and white checkerboard
        int width = 4, height = 4;
        std::vector<uint32_t> input(width * height);
        
        // Create checkerboard pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bool white = ((x + y) % 2 == 0);
                input[y * width + x] = white ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        
        std::cout << "4x4 checkerboard pattern:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t c = input[y * width + x];
                std::cout << (c == 0xFFFFFFFF ? "W" : "B");
            }
            std::cout << std::endl;
        }
        
        // Use identity transformation with BILINEAR interpolation and subpixel=true
        table.generate(width, height, 2, 2, "x", "y", true, true, // subpixel=true
                      audio_data, false, InterpolationMode::LINEAR);
        
        std::vector<uint32_t> output(width * height, 0xFF808080);
        
        // Apply transformation
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "\nOutput after bilinear interpolation:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t c = output[y * width + x];
                if (c == 0xFFFFFFFF) std::cout << "W";
                else if (c == 0xFF000000) std::cout << "B";
                else std::cout << "?";
            }
            std::cout << std::endl;
        }
        
        // Count corrupted pixels
        int corrupted = 0;
        std::vector<std::pair<int, uint32_t>> corrupted_pixels;
        
        for (int i = 0; i < width * height; i++) {
            uint32_t c = output[i];
            if (c != 0xFFFFFFFF && c != 0xFF000000) {
                corrupted++;
                corrupted_pixels.push_back({i, c});
            }
        }
        
        std::cout << "\nCorrupted pixels: " << corrupted << std::endl;
        
        if (corrupted > 0) {
            std::cout << "First few corrupted pixels:" << std::endl;
            for (size_t i = 0; i < std::min(corrupted_pixels.size(), (size_t)5); i++) {
                int pos = corrupted_pixels[i].first;
                uint32_t color = corrupted_pixels[i].second;
                int x = pos % width;
                int y = pos / width;
                
                uint8_t r = (color >> 16) & 0xFF;
                uint8_t g = (color >> 8) & 0xFF;
                uint8_t b = color & 0xFF;
                uint8_t a = (color >> 24) & 0xFF;
                
                std::cout << "  Pixel (" << x << "," << y << "): RGBA(" 
                          << (int)r << "," << (int)g << "," << (int)b << "," << (int)a << ")" << std::endl;
                std::cout << "    Expected: " << (((x + y) % 2 == 0) ? "WHITE" : "BLACK") << std::endl;
            }
        }
        
        // This should pass if bilinear interpolation is correct
        REQUIRE(corrupted == 0);
    }
}