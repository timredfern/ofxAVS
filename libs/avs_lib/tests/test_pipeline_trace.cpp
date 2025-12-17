#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>
#include <iomanip>

using namespace avs;

TEST_CASE("Pipeline Trace for Color Corruption", "[pipeline_trace]") {
    SECTION("Trace complete pipeline for specific pixels to find corruption source") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Create minimal test case that should expose corruption
        int width = 8, height = 8;
        std::vector<uint32_t> input(width * height, 0xFF000000); // Black
        
        // Put a few white pixels at strategic locations
        input[1 * width + 1] = 0xFFFFFFFF; // White at (1,1)
        input[3 * width + 3] = 0xFFFFFFFF; // White at (3,3)
        input[6 * width + 6] = 0xFFFFFFFF; // White at (6,6)
        
        std::cout << "8x8 test pattern with white pixels at (1,1), (3,3), (6,6):" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << (input[y * width + x] == 0xFFFFFFFF ? "W" : ".");
            }
            std::cout << std::endl;
        }
        
        // Use settings that might cause issues: small image, coarse grid
        table.generate(width, height, 3, 3, "x", "y-0.1", true, false, // Small movement
                      audio_data, false, InterpolationMode::LINEAR);
        
        std::vector<uint32_t> output(width * height, 0xFF808080);
        
        std::cout << "\\nTracing pipeline for key pixels..." << std::endl;
        
        // Manually trace the pipeline for a few pixels to see what's happening
        std::vector<std::pair<int, int>> trace_pixels = {{1, 1}, {3, 3}, {6, 6}, {2, 2}};
        
        for (auto pixel : trace_pixels) {
            int dest_x = pixel.first;
            int dest_y = pixel.second;
            
            std::cout << "\\n=== TRACING PIXEL (" << dest_x << "," << dest_y << ") ===" << std::endl;
            
            // Step 1: Calculate grid coordinates  
            double grid_x = (dest_x * (3 - 1.0)) / (width - 1.0);
            double grid_y = (dest_y * (3 - 1.0)) / (height - 1.0);
            std::cout << "1. Grid coords: (" << grid_x << ", " << grid_y << ")" << std::endl;
            
            // Step 2: Get interpolated source coordinates
            auto source_coords = table.get_interpolated_coordinates(grid_x, grid_y);
            std::cout << "2. Normalized source: (" << source_coords.first << ", " << source_coords.second << ")" << std::endl;
            
            // Step 3: Denormalize to pixel coordinates
            double src_x = source_coords.first * (width - 1);
            double src_y = source_coords.second * (height - 1);
            std::cout << "3. Pixel source: (" << src_x << ", " << src_y << ")" << std::endl;
            
            // Step 4: Clamp coordinates  
            double clamped_x = std::clamp(src_x, 0.0, (double)(width - 1));
            double clamped_y = std::clamp(src_y, 0.0, (double)(height - 1));
            std::cout << "4. Clamped: (" << clamped_x << ", " << clamped_y << ")" << std::endl;
            
            // Step 5: Check what will be sampled
            // For nearest neighbor (subpixel=false)
            int sample_x = (int)(clamped_x + 0.5);
            int sample_y = (int)(clamped_y + 0.5);
            sample_x = std::clamp(sample_x, 0, width - 1);
            sample_y = std::clamp(sample_y, 0, height - 1);
            
            uint32_t source_color = input[sample_y * width + sample_x];
            std::cout << "5. Will sample from pixel (" << sample_x << "," << sample_y << ")" << std::endl;
            std::cout << "6. Source color: 0x" << std::hex << source_color << std::dec;
            if (source_color == 0xFFFFFFFF) std::cout << " (WHITE)";
            else if (source_color == 0xFF000000) std::cout << " (BLACK)";
            else std::cout << " (OTHER)";
            std::cout << std::endl;
            
            // Check for coordinate corruption
            double coord_diff_x = abs(clamped_x - dest_x);
            double coord_diff_y = abs(clamped_y - dest_y);
            if (coord_diff_x > 2.0 || coord_diff_y > 2.0) {
                std::cout << "WARNING: Large coordinate displacement!" << std::endl;
            }
        }
        
        // Now run the actual apply function and compare
        table.apply(input.data(), output.data(), width, height, false);
        
        std::cout << "\\nActual output:" << std::endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t c = output[y * width + x];
                if (c == 0xFFFFFFFF) std::cout << "W";
                else if (c == 0xFF000000) std::cout << ".";
                else {
                    uint8_t r = (c >> 16) & 0xFF;
                    uint8_t g = (c >> 8) & 0xFF;
                    uint8_t b = c & 0xFF;
                    std::cout << "C";
                    
                    // Print the first corrupted pixel details
                    static bool first_corruption = true;
                    if (first_corruption) {
                        first_corruption = false;
                        std::cout << "\\nFIRST CORRUPTION at (" << x << "," << y << "): RGB(" 
                                  << (int)r << "," << (int)g << "," << (int)b << ")" << std::endl;
                    }
                }
            }
            std::cout << std::endl;
        }
        
        // Count corruption
        int corrupted = 0;
        for (int i = 0; i < width * height; i++) {
            uint32_t c = output[i];
            if (c != 0xFFFFFFFF && c != 0xFF000000) corrupted++;
        }
        
        std::cout << "Corrupted pixels: " << corrupted << std::endl;
        
        REQUIRE(corrupted == 0);
    }
}