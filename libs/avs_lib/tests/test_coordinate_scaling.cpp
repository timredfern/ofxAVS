#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Coordinate Scaling Issues", "[coord_scaling]") {
    SECTION("Test coordinate mapping for large image with coarse grid") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        int width = 64, height = 64;
        int grid_width = 16, grid_height = 16;
        
        // Create test pattern where we can track specific pixels
        std::vector<uint32_t> input(width * height, 0xFF000000); // Black background
        
        // Put colored markers at known positions
        input[10 * width + 10] = 0xFFFF0000; // Red at (10, 10)
        input[30 * width + 30] = 0xFF00FF00; // Green at (30, 30) 
        input[50 * width + 50] = 0xFF0000FF; // Blue at (50, 50)
        
        std::cout << "Testing 64x64 image with 16x16 grid coordinate mapping:" << std::endl;
        
        // Test identity transformation first
        table.generate(width, height, grid_width, grid_height, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        std::vector<uint32_t> output(width * height, 0xFF808080);
        table.apply(input.data(), output.data(), width, height, false);
        
        // Check specific marker positions
        uint32_t red_out = output[10 * width + 10];
        uint32_t green_out = output[30 * width + 30];
        uint32_t blue_out = output[50 * width + 50];
        
        std::cout << "Marker colors after identity transform:" << std::endl;
        std::cout << "Red (10,10): 0x" << std::hex << input[10 * width + 10] 
                  << " -> 0x" << red_out << std::dec << std::endl;
        std::cout << "Green (30,30): 0x" << std::hex << input[30 * width + 30] 
                  << " -> 0x" << green_out << std::dec << std::endl;
        std::cout << "Blue (50,50): 0x" << std::hex << input[50 * width + 50] 
                  << " -> 0x" << blue_out << std::dec << std::endl;
        
        // Manually check what coordinates are being computed
        std::cout << "\\nChecking coordinate calculations:" << std::endl;
        
        // Check a few key pixel positions
        std::vector<std::pair<int, int>> test_pixels = {{10, 10}, {30, 30}, {50, 50}};
        
        for (auto pixel : test_pixels) {
            int dest_x = pixel.first;
            int dest_y = pixel.second;
            
            // This is the calculation from apply() method
            double grid_x = (dest_x * (grid_width - 1.0)) / (width - 1.0);
            double grid_y = (dest_y * (grid_height - 1.0)) / (height - 1.0);
            
            std::cout << "Pixel (" << dest_x << "," << dest_y << ") maps to grid (" 
                      << grid_x << "," << grid_y << ")" << std::endl;
            
            // Get interpolated source coordinates
            auto source_coords = table.get_interpolated_coordinates(grid_x, grid_y);
            std::cout << "  Grid returns normalized coords: (" << source_coords.first 
                      << ", " << source_coords.second << ")" << std::endl;
            
            // Denormalize to pixel coordinates
            double src_pixel_x = source_coords.first * (width - 1);
            double src_pixel_y = source_coords.second * (height - 1);
            std::cout << "  Denormalized to pixel coords: (" << src_pixel_x 
                      << ", " << src_pixel_y << ")" << std::endl;
            
            // Check if this is reasonable for identity transform
            double x_diff = abs(src_pixel_x - dest_x);
            double y_diff = abs(src_pixel_y - dest_y);
            std::cout << "  Difference from identity: (" << x_diff << ", " << y_diff << ")" << std::endl;
            
            if (x_diff > 5 || y_diff > 5) {
                std::cout << "  WARNING: Large coordinate difference!" << std::endl;
            }
        }
        
        // For identity transform, colors should be approximately preserved
        bool red_preserved = (red_out & 0x00FF0000) > 0x00800000;
        bool green_preserved = (green_out & 0x0000FF00) > 0x00008000;
        bool blue_preserved = (blue_out & 0x000000FF) > 0x00000080;
        
        std::cout << "Color preservation: Red=" << red_preserved 
                  << " Green=" << green_preserved << " Blue=" << blue_preserved << std::endl;
        
        REQUIRE(red_preserved);
        REQUIRE(green_preserved);
        REQUIRE(blue_preserved);
    }
    
    SECTION("Test edge coordinate clamping behavior") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Test what happens at image edges
        int width = 10, height = 10;
        std::vector<uint32_t> input(width * height, 0xFF808080);
        
        // Put distinct colors at edges
        input[0] = 0xFFFF0000;                    // Top-left
        input[width-1] = 0xFF00FF00;              // Top-right
        input[(height-1)*width] = 0xFF0000FF;     // Bottom-left
        input[height*width-1] = 0xFFFFFFFF;       // Bottom-right
        
        std::vector<uint32_t> output(width * height, 0xFF000000);
        
        // Use coarse grid
        table.generate(width, height, 3, 3, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "\\nEdge clamping test (10x10 with 3x3 grid):" << std::endl;
        std::cout << "Corners - Input vs Output:" << std::endl;
        std::cout << "Top-left: 0x" << std::hex << input[0] << " -> 0x" << output[0] << std::dec << std::endl;
        std::cout << "Top-right: 0x" << std::hex << input[width-1] << " -> 0x" << output[width-1] << std::dec << std::endl;
        std::cout << "Bottom-left: 0x" << std::hex << input[(height-1)*width] << " -> 0x" << output[(height-1)*width] << std::dec << std::endl;
        std::cout << "Bottom-right: 0x" << std::hex << input[height*width-1] << " -> 0x" << output[height*width-1] << std::dec << std::endl;
        
        REQUIRE(true); // Just for debug output
    }
}