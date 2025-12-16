#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Identity Transformation Only", "[identity]") {
    SECTION("Exact grid 1x1 - trivial case") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Simplest possible case: 1x1 pixel with 1x1 grid
        int width = 1, height = 1;
        
        table.generate(width, height, 1, 1, "x", "y", true, false, 
                      audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input = {0xFFFF0000};  // Red pixel
        std::vector<uint32_t> output = {0xFF000000}; // Black pixel
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "1x1 test: input=" << std::hex << input[0] 
                  << " output=" << output[0] << std::dec << std::endl;
        
        REQUIRE(output[0] == 0xFFFF0000);
    }
    
    SECTION("Exact grid 2x2") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // 2x2 pixel with 2x2 grid - should be exact mapping
        int width = 2, height = 2;
        
        table.generate(width, height, width, height, "x", "y", true, false,
                      audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input = {
            0xFFFF0000, 0xFF00FF00,  // Red, Green
            0xFF0000FF, 0xFFFFFFFF   // Blue, White
        };
        std::vector<uint32_t> output(4, 0xFF000000);
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "2x2 Input:  " << std::hex << input[0] << " " << input[1] << std::endl;
        std::cout << "            " << std::hex << input[2] << " " << input[3] << std::endl;
        std::cout << "2x2 Output: " << std::hex << output[0] << " " << output[1] << std::endl;
        std::cout << "            " << std::hex << output[2] << " " << output[3] << std::dec << std::endl;
        
        // Should be identical for exact grid identity
        bool matches = true;
        for (int i = 0; i < 4; i++) {
            if (input[i] != output[i]) {
                std::cout << "Mismatch at " << i << ": " << std::hex 
                          << input[i] << " != " << output[i] << std::dec << std::endl;
                matches = false;
            }
        }
        
        REQUIRE(matches);
    }
}