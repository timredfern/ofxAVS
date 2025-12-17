#include <catch2/catch_test_macros.hpp>
#include "../effects/dynamic_movement_effect.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Exact ofxAVS Scenario", "[exact_ofxavs]") {
    SECTION("Replicate exact ofxAVS oscilloscope + dynamic movement chain") {
        // This test replicates exactly what happens in ofxAVS example
        
        // Create oscilloscope-like output: black background with white oscilloscope trace
        int width = 512, height = 512; // Typical ofxAVS window size
        std::vector<uint32_t> oscilloscope_output(width * height, 0xFF000000);
        
        // Draw white oscilloscope line (horizontal with sine wave variation)
        int center_y = height / 2;
        for (int x = 0; x < width; x++) {
            int y = center_y + (int)(50 * sin(x * 0.02)); // Sine wave
            if (y >= 0 && y < height) {
                oscilloscope_output[y * width + x] = 0xFFFFFFFF; // Pure white
            }
        }
        
        // Add a few isolated white pixels (like oscilloscope dots)
        oscilloscope_output[100 * width + 100] = 0xFFFFFFFF;
        oscilloscope_output[200 * width + 300] = 0xFFFFFFFF;
        oscilloscope_output[400 * width + 200] = 0xFFFFFFFF;
        
        std::cout << "Created 512x512 oscilloscope pattern" << std::endl;
        
        // Count white pixels in input
        int white_pixels_in = 0;
        for (int i = 0; i < width * height; i++) {
            if (oscilloscope_output[i] == 0xFFFFFFFF) white_pixels_in++;
        }
        std::cout << "Input white pixels: " << white_pixels_in << std::endl;
        
        // Now apply dynamic movement effect with EXACT ofxAVS settings
        DynamicMovementEffect movement;
        
        // Configure exactly like ofxAVS example  
        movement.parameters().set_string("pixel_script", "x=x; y=y-0.01");
        movement.parameters().set_bool("rectangular", true);
        movement.parameters().set_int("grid_width", 16);
        movement.parameters().set_int("grid_height", 16); 
        movement.parameters().set_int("interpolation", 1); // LINEAR (default in ofxAVS)
        movement.parameters().set_bool("wrap", false);
        movement.parameters().set_bool("blend", false);
        
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        std::vector<uint32_t> movement_output(width * height, 0xFF000000);
        
        // Apply the movement effect
        int result = movement.render(audio_data, 0, 
                                   oscilloscope_output.data(), movement_output.data(), 
                                   width, height);
        
        std::cout << "Dynamic movement effect returned: " << result << std::endl;
        
        // Analyze the output for color corruption
        int white_out = 0, black_out = 0, colored_out = 0;
        std::vector<std::pair<int, uint32_t>> corrupted_pixels;
        
        for (int i = 0; i < width * height; i++) {
            uint32_t color = movement_output[i];
            
            if (color == 0xFFFFFFFF) {
                white_out++;
            } else if (color == 0xFF000000) {
                black_out++;
            } else {
                colored_out++;
                corrupted_pixels.push_back({i, color});
            }
        }
        
        std::cout << "Output analysis:" << std::endl;
        std::cout << "  White pixels: " << white_out << std::endl;
        std::cout << "  Black pixels: " << black_out << std::endl;
        std::cout << "  COLORED pixels: " << colored_out << std::endl;
        
        if (colored_out > 0) {
            std::cout << "\\nFirst 10 corrupted pixels:" << std::endl;
            for (size_t i = 0; i < std::min(corrupted_pixels.size(), (size_t)10); i++) {
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
            }
        }
        
        // This is the critical test: there should be NO colored pixels
        // when resampling pure white on black background
        REQUIRE(colored_out == 0);
        
        // Also verify we didn't lose all the white pixels
        REQUIRE(white_out > 0);
    }
}