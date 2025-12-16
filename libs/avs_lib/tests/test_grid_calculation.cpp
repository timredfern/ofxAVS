#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using Catch::Approx;
using namespace avs;

TEST_CASE("Grid Calculation Verification", "[grid_calc]") {
    SECTION("Test coordinate calculation manually") {
        // Replicate the exact calculation from apply() method
        int width = 4, height = 4;
        int grid_width = 2, grid_height = 2;
        
        std::cout << "=== Grid coordinate calculation for 4x4 image, 2x2 grid ===" << std::endl;
        
        for (int dest_y = 0; dest_y < height; dest_y++) {
            for (int dest_x = 0; dest_x < width; dest_x++) {
                // This is the exact calculation from line 238-239
                double grid_x = (dest_x * (grid_width - 1.0)) / (width - 1.0);
                double grid_y = (dest_y * (grid_height - 1.0)) / (height - 1.0);
                
                // This is the nearest neighbor calculation from line 186
                int gx = (int)(grid_x + 0.5);
                int gy = (int)(grid_y + 0.5);
                gx = std::clamp(gx, 0, grid_width - 1);
                gy = std::clamp(gy, 0, grid_height - 1);
                
                std::cout << "Pixel(" << dest_x << "," << dest_y << ") -> grid(" 
                          << grid_x << "," << grid_y << ") -> nearest(" << gx << "," << gy << ")" << std::endl;
            }
        }
        
        REQUIRE(true); // Just for analysis
    }
    
    SECTION("Test actual coordinate grid values") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Generate identity transformation
        table.generate(4, 4, 2, 2, "x", "y", true, false,
                      audio_data, false, InterpolationMode::NONE);
        
        // The coordinate grid should contain the transformed coordinates
        // For 2x2 grid with identity "x", "y":
        // Grid(0,0) should contain normalized coordinates for what was input at (0,0) → (0,0)
        // Grid(1,0) should contain normalized coordinates for what was input at (1,0) → (1,0)  
        // Grid(0,1) should contain normalized coordinates for what was input at (0,1) → (0,1)
        // Grid(1,1) should contain normalized coordinates for what was input at (1,1) → (1,1)
        
        std::cout << "\n=== Grid values after identity transformation ===" << std::endl;
        
        // Test with a simple pattern to see where pixels actually go
        std::vector<uint32_t> input(16);
        std::vector<uint32_t> output(16, 0xFF000000);
        
        // Put unique colors at grid corners
        std::fill(input.begin(), input.end(), 0xFF808080); // Gray background
        input[0] = 0xFFFF0000;   // Red at (0,0) 
        input[3] = 0xFF00FF00;   // Green at (3,0)
        input[12] = 0xFF0000FF;  // Blue at (0,3)
        input[15] = 0xFFFFFFFF;  // White at (3,3)
        
        table.apply(input.data(), output.data(), 4, 4, false);
        
        std::cout << "Input corners: Red(0,0) Green(3,0) Blue(0,3) White(3,3)" << std::endl;
        std::cout << "Output pattern:" << std::endl;
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                uint32_t color = output[y * 4 + x];
                char c = 'G'; // Gray
                if ((color & 0x00FF0000) > 0x00800000) c = 'R'; // Red
                else if ((color & 0x0000FF00) > 0x00008000) c = 'G'; // Green  
                else if ((color & 0x000000FF) > 0x00000080) c = 'B'; // Blue
                else if ((color & 0x00FFFFFF) > 0x00C0C0C0) c = 'W'; // White
                std::cout << c << " ";
            }
            std::cout << std::endl;
        }
        
        // For correct identity transformation, corners should stay in corners
        REQUIRE((output[0] & 0x00FF0000) > 0x00800000);   // Red at (0,0)
        REQUIRE((output[3] & 0x0000FF00) > 0x00008000);   // Green at (3,0) 
        REQUIRE((output[12] & 0x000000FF) > 0x00000080);  // Blue at (0,3)
        REQUIRE((output[15] & 0x00FFFFFF) > 0x00C0C0C0);  // White at (3,3)
    }
}