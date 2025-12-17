#include <catch2/catch_test_macros.hpp>
#include "../effects/dynamic_movement_effect.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Dynamic Movement Realistic Test", "[dynamic_realistic]") {
    SECTION("Test actual dynamic movement effect with oscilloscope-like input") {
        DynamicMovementEffect effect;
        
        // Configure like the ofxAVS example
        effect.parameters().set_string("pixel_script", "x=x; y=y-0.01");
        effect.parameters().set_bool("rectangular", true);
        effect.parameters().set_int("grid_width", 16);
        effect.parameters().set_int("grid_height", 16);
        effect.parameters().set_int("interpolation", 1); // LINEAR
        
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create a test pattern similar to what oscilloscope might produce
        int width = 64, height = 64;
        std::vector<uint32_t> input(width * height, 0xFF000000); // Black background
        std::vector<uint32_t> output(width * height, 0xFF000000);
        
        // Draw some oscilloscope-like lines (horizontal line in middle with some variation)
        for (int x = 0; x < width; x++) {
            int y = height/2 + (int)(5 * sin(x * 0.1)); // Sine wave
            if (y >= 0 && y < height) {
                input[y * width + x] = 0xFFFFFFFF; // White line
            }
        }
        
        // Add some colored elements to detect artifacts
        input[10 * width + 10] = 0xFFFF0000; // Red dot
        input[50 * width + 50] = 0xFF00FF00; // Green dot
        
        std::cout << "Testing dynamic movement with 64x64 oscilloscope-like pattern" << std::endl;
        
        // Run the effect
        int result = effect.render(audio_data, 0, input.data(), output.data(), width, height);
        
        std::cout << "Effect returned: " << result << std::endl;
        
        // Check if colored dots preserved or changed dramatically
        uint32_t red_dot_out = output[10 * width + 10];
        uint32_t green_dot_out = output[50 * width + 50];
        
        std::cout << "Red dot: 0x" << std::hex << input[10 * width + 10] 
                  << " -> 0x" << red_dot_out << std::dec << std::endl;
        std::cout << "Green dot: 0x" << std::hex << input[50 * width + 50] 
                  << " -> 0x" << green_dot_out << std::dec << std::endl;
        
        // Check for unexpected color changes
        uint32_t red_in = (input[10 * width + 10] >> 16) & 0xFF;
        uint32_t red_out = (red_dot_out >> 16) & 0xFF;
        uint32_t green_in = (input[50 * width + 50] >> 8) & 0xFF;
        uint32_t green_out = (green_dot_out >> 8) & 0xFF;
        
        std::cout << "Red channel: " << red_in << " -> " << red_out << std::endl;
        std::cout << "Green channel: " << green_in << " -> " << green_out << std::endl;
        
        // With small movement, colors should be approximately preserved
        REQUIRE(abs((int)red_in - (int)red_out) <= 50);
        REQUIRE(abs((int)green_in - (int)green_out) <= 50);
        
        REQUIRE(result == 1);
    }
    
    SECTION("Test with different grid sizes to isolate grid-related artifacts") {
        DynamicMovementEffect effect;
        
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Test pattern with distinct colors
        std::vector<uint32_t> input = {
            0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFFFF,
            0xFF800000, 0xFF008000, 0xFF000080, 0xFF808080,
            0xFF400000, 0xFF004000, 0xFF000040, 0xFF404040,
            0xFF000000, 0xFF200000, 0xFF002000, 0xFF000020
        };
        std::vector<uint32_t> output(16, 0xFF000000);
        
        std::vector<int> grid_sizes = {2, 4, 8};
        
        for (int grid_size : grid_sizes) {
            std::cout << "\nTesting with " << grid_size << "x" << grid_size << " grid:" << std::endl;
            
            effect.parameters().set_string("pixel_script", "x=x; y=y"); // Identity
            effect.parameters().set_bool("rectangular", true);
            effect.parameters().set_int("grid_width", grid_size);
            effect.parameters().set_int("grid_height", grid_size);
            effect.parameters().set_int("interpolation", 1); // LINEAR
            
            std::fill(output.begin(), output.end(), 0xFF000000);
            
            int result = effect.render(audio_data, 0, input.data(), output.data(), 4, 4);
            
            std::cout << "Input:  ";
            for (int i = 0; i < 4; i++) std::cout << std::hex << input[i] << " ";
            std::cout << std::endl << "Output: ";
            for (int i = 0; i < 4; i++) std::cout << std::hex << output[i] << " ";
            std::cout << std::dec << std::endl;
            
            // Count exact matches
            int matches = 0;
            for (int i = 0; i < 16; i++) {
                if (input[i] == output[i]) matches++;
            }
            std::cout << "Exact matches: " << matches << "/16" << std::endl;
            
            // Identity transformation should preserve most colors
            if (grid_size >= 4) {
                REQUIRE(matches >= 4); // With exact grid, should be better
            }
        }
    }
}