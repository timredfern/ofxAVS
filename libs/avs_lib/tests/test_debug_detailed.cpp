#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Detailed Debug Analysis", "[debug_detailed]") {
    SECTION("Step by step coordinate generation debug") {
        // Let's create our own simple test to understand the grid generation
        
        // Create a simple 2x2 image with 2x2 grid (exact match)
        std::vector<uint32_t> input = {
            0xFFFF0000, 0xFF00FF00,  // Red, Green
            0xFF0000FF, 0xFFFFFFFF   // Blue, White  
        };
        std::vector<uint32_t> output(4, 0xFF000000);
        
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Test different expression combinations
        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"x", "y"},           // Simple identity
            {"0.5", "0.5"},      // Constant
            {"x=x; y=y", "x=x; y=y"}  // Multi-statement
        };
        
        for (auto& test_case : test_cases) {
            std::cout << "\nTesting expressions: x='" << test_case.first 
                      << "' y='" << test_case.second << "'" << std::endl;
            
            table.generate(2, 2, 2, 2, test_case.first, test_case.second, true, false,
                          audio_data, false, InterpolationMode::LINEAR);
            
            if (!table.is_valid()) {
                std::cout << "  Table is INVALID!" << std::endl;
                continue;
            }
            
            // Test all grid coordinates
            for (int gy = 0; gy <= 1; gy++) {
                for (int gx = 0; gx <= 1; gx++) {
                    auto coords = table.get_interpolated_coordinates(gx, gy);
                    std::cout << "  Grid(" << gx << "," << gy << ") -> (" 
                              << coords.first << ", " << coords.second << ")" << std::endl;
                }
            }
            
            // Apply transformation
            std::fill(output.begin(), output.end(), 0xFF000000);
            table.apply(input.data(), output.data(), 2, 2, false);
            
            // Check if transformation preserves anything
            bool any_match = false;
            for (int i = 0; i < 4; i++) {
                if (output[i] == input[i]) {
                    any_match = true;
                    break;
                }
            }
            std::cout << "  Any pixels preserved: " << (any_match ? "YES" : "NO") << std::endl;
        }
        
        REQUIRE(true); // This test is just for debugging output
    }
}