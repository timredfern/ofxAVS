#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <iostream>

using namespace avs;

TEST_CASE("Sample Pixel Bug Investigation", "[sample_pixel_bug]") {
    SECTION("Test sample_pixel function directly with black/white pattern") {
        CoordinateLookupTable table;
        
        // Create simple 4x4 test pattern: black with one white pixel
        int width = 4, height = 4;
        std::vector<uint32_t> input(width * height, 0xFF000000); // All black
        input[1 * width + 1] = 0xFFFFFFFF; // White at (1,1)
        input[2 * width + 2] = 0xFFFFFFFF; // White at (2,2)
        
        std::cout << "4x4 pattern:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t c = input[y * width + x];
                std::cout << (c == 0xFFFFFFFF ? "W" : ".");
            }
            std::cout << std::endl;
        }
        
        // Initialize table for sampling (we need output_width_ and output_height_ set)
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        table.generate(width, height, 2, 2, "x", "y", true, false, audio_data, false, InterpolationMode::LINEAR);
        
        // Test sampling at various coordinates
        std::vector<std::pair<double, double>> test_coords = {
            {0.0, 0.0},   // Should sample black
            {1.0, 1.0},   // Should sample white
            {2.0, 2.0},   // Should sample white  
            {1.5, 1.5},   // Between white pixels
            {0.5, 0.5},   // Near black
            {1.1, 1.1},   // Near white
        };
        
        for (auto coord : test_coords) {
            double x = coord.first;
            double y = coord.second;
            
            std::cout << "\nSampling at (" << x << ", " << y << "):" << std::endl;
            
            // Test both subpixel modes
            for (bool subpixel : {false, true}) {
                std::cout << "  Subpixel=" << (subpixel ? "true" : "false") << ": ";
                
                // We need to access the private sample_pixel method through apply
                // Create a temporary table with the subpixel setting
                CoordinateLookupTable test_table;
                test_table.generate(width, height, 2, 2, "x", "y", true, subpixel, audio_data, false, InterpolationMode::LINEAR);
                
                // Use a 1x1 output to sample just this coordinate
                std::vector<uint32_t> output(1, 0xFF808080);
                
                // Manually call the internal sampling by creating a grid that maps to our test coordinate
                // This is tricky - let me create a test pattern that forces sampling at the exact coordinate we want
                
                // Instead, let's create a test that exposes the bug more directly
                if (subpixel) {
                    // For subpixel sampling, check if pure black/white gets contaminated
                    if (x == 1.0 && y == 1.0) {
                        // This should sample pure white, but might get contaminated by bilinear interpolation
                        std::cout << "Should be pure white";
                    }
                } else {
                    // For nearest neighbor, should always be pure
                    std::cout << "Should be pure (nearest neighbor)";
                }
                
                std::cout << std::endl;
            }
        }
        
        // The key insight: even with subpixel=false, if coordinates aren't exactly integers,
        // we might still have precision issues in the rounding
        
        std::cout << "\nTesting edge case coordinates that might cause rounding errors:" << std::endl;
        
        // Test coordinates that are almost integers
        std::vector<double> edge_coords = {0.999999, 1.000001, 1.499999, 1.500001};
        
        for (double test_x : edge_coords) {
            for (double test_y : edge_coords) {
                int ix = (int)(test_x + 0.5);
                int iy = (int)(test_y + 0.5);
                std::cout << "(" << test_x << ", " << test_y << ") -> nearest neighbor (" << ix << ", " << iy << ")" << std::endl;
            }
        }
    }
}