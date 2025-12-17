#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Pixel Sampling Artifacts", "[pixel_sampling]") {
    SECTION("Test identity transformation with colored corners") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Use a 4x4 image with distinct corner colors
        int width = 4, height = 4;
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0xFF000000);
        
        // Create a pattern with distinct corner colors to detect sampling errors
        std::fill(input.begin(), input.end(), 0xFF808080); // Gray background
        
        input[0] = 0xFFFF0000;   // Red at (0,0) - top-left
        input[3] = 0xFF00FF00;   // Green at (3,0) - top-right
        input[12] = 0xFF0000FF;  // Blue at (0,3) - bottom-left  
        input[15] = 0xFFFFFFFF;  // White at (3,3) - bottom-right
        
        std::cout << "Input pattern (corners should be preserved):" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t color = input[y * width + x];
                char c = 'G'; // Gray
                if ((color & 0x00FF0000) == 0x00FF0000 && (color & 0x0000FFFF) == 0) c = 'R'; // Pure red
                else if ((color & 0x0000FF00) == 0x0000FF00 && (color & 0x00FF00FF) == 0) c = 'G'; // Pure green
                else if ((color & 0x000000FF) == 0x000000FF && (color & 0x00FFFF00) == 0) c = 'B'; // Pure blue
                else if ((color & 0x00FFFFFF) == 0x00FFFFFF) c = 'W'; // White
                else c = '.'; // Gray
                std::cout << c << " ";
            }
            std::cout << std::endl;
        }
        
        // Test with LINEAR interpolation and identity transformation
        table.generate(width, height, 2, 2, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "\\nOutput pattern with identity transformation:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t color = output[y * width + x];
                char c = 'G'; // Gray
                if ((color & 0x00FF0000) == 0x00FF0000 && (color & 0x0000FFFF) == 0) c = 'R'; // Pure red
                else if ((color & 0x0000FF00) == 0x0000FF00 && (color & 0x00FF00FF) == 0) c = 'G'; // Pure green
                else if ((color & 0x000000FF) == 0x000000FF && (color & 0x00FFFF00) == 0) c = 'B'; // Pure blue
                else if ((color & 0x00FFFFFF) == 0x00FFFFFF) c = 'W'; // White
                else {
                    // Show mixed colors with lowercase letters
                    if ((color & 0x00FF0000) > 0x00800000) c = 'r'; // Reddish
                    else if ((color & 0x0000FF00) > 0x00008000) c = 'g'; // Greenish
                    else if ((color & 0x000000FF) > 0x00000080) c = 'b'; // Blueish
                    else c = '.'; // Gray
                }
                std::cout << c << " ";
            }
            std::cout << std::endl;
        }
        
        // Check if corners are preserved correctly
        bool corners_correct = true;
        
        // Check corner colors - they should be exactly preserved for identity transform
        uint32_t out_tl = output[0];   // top-left
        uint32_t out_tr = output[3];   // top-right
        uint32_t out_bl = output[12];  // bottom-left
        uint32_t out_br = output[15];  // bottom-right
        
        std::cout << "\\nCorner color analysis:" << std::endl;
        std::cout << "Top-left: input=0x" << std::hex << input[0] << " output=0x" << out_tl << std::dec << std::endl;
        std::cout << "Top-right: input=0x" << std::hex << input[3] << " output=0x" << out_tr << std::dec << std::endl;
        std::cout << "Bottom-left: input=0x" << std::hex << input[12] << " output=0x" << out_bl << std::dec << std::endl;
        std::cout << "Bottom-right: input=0x" << std::hex << input[15] << " output=0x" << out_br << std::dec << std::endl;
        
        if (out_tl != input[0]) {
            std::cout << "ERROR: Top-left corner color changed!" << std::endl;
            corners_correct = false;
        }
        if (out_tr != input[3]) {
            std::cout << "ERROR: Top-right corner color changed!" << std::endl;
            corners_correct = false;
        }
        if (out_bl != input[12]) {
            std::cout << "ERROR: Bottom-left corner color changed!" << std::endl;
            corners_correct = false;
        }
        if (out_br != input[15]) {
            std::cout << "ERROR: Bottom-right corner color changed!" << std::endl;
            corners_correct = false;
        }
        
        REQUIRE(corners_correct);
    }
    
    SECTION("Test pixel sampling bounds directly") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create a simple test pattern
        std::vector<uint32_t> input = {
            0xFFFF0000, 0xFF00FF00,  // Red, Green
            0xFF0000FF, 0xFFFFFFFF   // Blue, White
        };
        std::vector<uint32_t> output(4, 0xFF000000);
        
        // Test with identity transformation on 2x2 image
        table.generate(2, 2, 2, 2, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), 2, 2, false);
        
        std::cout << "\\n2x2 test - Input vs Output:" << std::endl;
        for (int i = 0; i < 4; i++) {
            std::cout << "Pixel " << i << ": 0x" << std::hex << input[i] 
                      << " -> 0x" << output[i] << std::dec << std::endl;
            
            // For 2x2 identity, output should exactly match input
            if (input[i] != output[i]) {
                std::cout << "ERROR: Pixel " << i << " color changed in identity transform!" << std::endl;
            }
        }
        
        // All pixels should be exactly preserved
        bool all_preserved = true;
        for (int i = 0; i < 4; i++) {
            if (input[i] != output[i]) {
                all_preserved = false;
                break;
            }
        }
        
        REQUIRE(all_preserved);
    }
}