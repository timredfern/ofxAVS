#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <iostream>
#include <iomanip>

using namespace avs;

TEST_CASE("Interpolate Pixels Edge Cases", "[interpolate_edge_cases]") {
    SECTION("Test edge cases that could cause color corruption") {
        CoordinateLookupTable table;
        
        // Test case 1: All four pixels are the same white color
        // This should ALWAYS return exactly white, no matter what fx/fy values
        std::cout << "Testing all-white interpolation:" << std::endl;
        
        std::vector<std::pair<double, double>> test_coords = {
            {0.0, 0.0}, {0.0, 1.0}, {1.0, 0.0}, {1.0, 1.0},  // corners
            {0.5, 0.5},  // center
            {0.1, 0.9}, {0.9, 0.1}, {0.3, 0.7},  // off-center
            {0.000001, 0.000001}, {0.999999, 0.999999}  // near edges
        };
        
        for (auto coord : test_coords) {
            double fx = coord.first;
            double fy = coord.second;
            
            uint32_t result = table.interpolate_pixels(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, fx, fy);
            
            std::cout << "  fx=" << std::fixed << std::setprecision(6) << fx 
                     << ", fy=" << fy << " -> 0x" << std::hex << result << std::dec << std::endl;
                     
            if (result != 0xFFFFFFFF) {
                std::cout << "    *** BUG: All white pixels should interpolate to white! ***" << std::endl;
                
                uint8_t r = (result >> 16) & 0xFF;
                uint8_t g = (result >> 8) & 0xFF;
                uint8_t b = result & 0xFF;
                uint8_t a = (result >> 24) & 0xFF;
                
                std::cout << "    Got RGBA(" << (int)r << "," << (int)g << "," << (int)b << "," << (int)a 
                          << ") instead of (255,255,255,255)" << std::endl;
            }
            
            REQUIRE(result == 0xFFFFFFFF);
        }
        
        // Test case 2: All four pixels are the same black color  
        std::cout << "\nTesting all-black interpolation:" << std::endl;
        
        for (auto coord : test_coords) {
            double fx = coord.first;
            double fy = coord.second;
            
            uint32_t result = table.interpolate_pixels(0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, fx, fy);
            
            if (result != 0xFF000000) {
                std::cout << "  fx=" << fx << ", fy=" << fy << " -> 0x" << std::hex << result << std::dec;
                std::cout << " *** BUG: All black pixels should interpolate to black! ***" << std::endl;
            }
            
            REQUIRE(result == 0xFF000000);
        }
        
        // Test case 3: Checkerboard pattern that should create gray in center
        std::cout << "\nTesting checkerboard interpolation at center:" << std::endl;
        
        uint32_t result = table.interpolate_pixels(0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0.5, 0.5);
        std::cout << "Checkerboard center result: 0x" << std::hex << result << std::dec << std::endl;
        
        uint8_t r = (result >> 16) & 0xFF;
        uint8_t g = (result >> 8) & 0xFF;
        uint8_t b = result & 0xFF;
        uint8_t a = (result >> 24) & 0xFF;
        
        std::cout << "RGBA(" << (int)r << "," << (int)g << "," << (int)b << "," << (int)a << ")" << std::endl;
        
        // Should be 50% gray
        REQUIRE(r == 128);
        REQUIRE(g == 128);
        REQUIRE(b == 128);
        REQUIRE(a == 255);
        
        // Test case 4: Edge case - what happens with extreme coordinate values?
        std::cout << "\nTesting extreme coordinate values:" << std::endl;
        
        std::vector<std::pair<double, double>> extreme_coords = {
            {-0.1, 0.5}, {1.1, 0.5}, {0.5, -0.1}, {0.5, 1.1}
        };
        
        for (auto coord : extreme_coords) {
            double fx = coord.first;
            double fy = coord.second;
            
            // With all white pixels, should still be white even with extreme coords
            uint32_t result = table.interpolate_pixels(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, fx, fy);
            
            std::cout << "  Extreme fx=" << fx << ", fy=" << fy << " -> 0x" << std::hex << result << std::dec;
            
            if (result != 0xFFFFFFFF) {
                std::cout << " *** POTENTIAL BUG WITH EXTREME COORDINATES ***";
            }
            std::cout << std::endl;
        }
    }
}