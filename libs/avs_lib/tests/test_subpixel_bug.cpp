#include <catch2/catch_test_macros.hpp>
#include "../effects/dynamic_movement_effect.h"
#include <cstring>
#include <iostream>
#include <iomanip>

using namespace avs;

TEST_CASE("Subpixel Bug Investigation", "[subpixel_bug]") {
    SECTION("Check if subpixel=true is accidentally being set") {
        DynamicMovementEffect movement;
        
        // Configure exactly like ofxAVS
        movement.parameters().set_string("pixel_script", "x=x; y=y-0.01");
        movement.parameters().set_bool("rectangular", true);
        movement.parameters().set_int("grid_width", 16);
        movement.parameters().set_int("grid_height", 16); 
        movement.parameters().set_int("interpolation", 1); // LINEAR
        movement.parameters().set_bool("wrap", false);
        movement.parameters().set_bool("blend", false);
        
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create simple black and white pattern  
        int width = 64, height = 64;
        std::vector<uint32_t> input(width * height, 0xFF000000);
        
        // Add some white pixels
        for (int i = 0; i < 100; i++) {
            input[i * 13 % (width * height)] = 0xFFFFFFFF;
        }
        
        std::vector<uint32_t> output(width * height, 0xFF808080);
        
        // Apply the movement effect
        int result = movement.render(audio_data, 0, 
                                   input.data(), output.data(), 
                                   width, height);
                                   
        std::cout << "Movement effect returned: " << result << std::endl;
        
        // Check output for corruption
        int corrupted = 0;
        for (int i = 0; i < width * height; i++) {
            uint32_t c = output[i];
            if (c != 0xFFFFFFFF && c != 0xFF000000) {
                corrupted++;
                if (corrupted <= 5) {
                    uint8_t r = (c >> 16) & 0xFF;
                    uint8_t g = (c >> 8) & 0xFF;
                    uint8_t b = c & 0xFF;
                    uint8_t a = (c >> 24) & 0xFF;
                    
                    std::cout << "Corrupted pixel " << corrupted << ": RGBA(" 
                              << (int)r << "," << (int)g << "," << (int)b << "," << (int)a << ")" << std::endl;
                }
            }
        }
        
        std::cout << "Total corrupted pixels: " << corrupted << std::endl;
        
        if (corrupted > 0) {
            std::cout << "*** BUG FOUND: Pixels corrupted when they should be pure ***" << std::endl;
            
            // Let's also check what the grid table's subpixel setting is
            // We can't access it directly, but we can infer it
            std::cout << "Checking if issue is in the grid generation..." << std::endl;
        }
        
        REQUIRE(corrupted == 0);
    }
    
    SECTION("Test if fractional coordinates are being generated") {
        // Create a custom coordinate table that should generate integer coordinates
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        int width = 8, height = 8;
        std::vector<uint32_t> input(width * height);
        
        // Checkerboard pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bool white = ((x + y) % 2 == 0);
                input[y * width + x] = white ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        
        // Identity transformation should produce integer coordinates
        table.generate(width, height, 2, 2, "x", "y", true, false, // subpixel=false
                      audio_data, false, InterpolationMode::LINEAR);
        
        std::vector<uint32_t> output(width * height, 0xFF808080);
        
        // This should be a perfect copy with no interpolation
        table.apply(input.data(), output.data(), width, height, false);
        
        // Check for any differences
        int differences = 0;
        for (int i = 0; i < width * height; i++) {
            if (input[i] != output[i]) {
                differences++;
                std::cout << "Pixel " << i << " changed from 0x" << std::hex << input[i] 
                          << " to 0x" << output[i] << std::dec << std::endl;
            }
        }
        
        std::cout << "Total differences in identity transform: " << differences << std::endl;
        
        REQUIRE(differences == 0);
    }
}