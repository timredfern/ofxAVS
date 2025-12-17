#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <iostream>
#include <iomanip>

using namespace avs;

// Helper function to test interpolate_pixels directly
uint32_t test_interpolate_pixels(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11, double fx, double fy) {
    CoordinateLookupTable table;
    return table.interpolate_pixels(p00, p01, p10, p11, fx, fy);
}

TEST_CASE("Interpolate Pixels Precision Bug", "[interpolate_precision]") {
    SECTION("Test floating point precision in pixel interpolation") {
        
        std::cout << std::fixed << std::setprecision(10);
        
        // Test cases that should produce pure colors but might have precision errors
        struct TestCase {
            std::string name;
            uint32_t p00, p01, p10, p11;
            double fx, fy;
            uint32_t expected;
        };
        
        std::vector<TestCase> test_cases = {
            // Pure corners (should always be exact)
            {"Top-left corner", 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0.0, 0.0, 0xFF000000},
            {"Top-right corner", 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 1.0, 0.0, 0xFFFFFFFF},
            {"Bottom-left corner", 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0.0, 1.0, 0xFFFFFFFF},
            {"Bottom-right corner", 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 1.0, 1.0, 0xFF000000},
            
            // Edge cases that might have precision issues
            {"Very small fx", 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0.0000001, 0.0, 0xFF000000},
            {"Very large fx", 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0.9999999, 0.0, 0xFFFFFFFF},
            {"Very small fy", 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0.0, 0.0000001, 0xFF000000},
            {"Very large fy", 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0.0, 0.9999999, 0xFF000000},
            
            // All same color (should always be exact)
            {"All black", 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0.5, 0.5, 0xFF000000},
            {"All white", 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0.5, 0.5, 0xFFFFFFFF},
            
            // Center of different patterns
            {"Center of checkerboard", 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0.5, 0.5, 0xFF808080},
        };
        
        for (const auto& test : test_cases) {
            std::cout << "\nTesting: " << test.name << std::endl;
            std::cout << "  fx=" << test.fx << ", fy=" << test.fy << std::endl;
            
            uint32_t result = test_interpolate_pixels(test.p00, test.p01, test.p10, test.p11, test.fx, test.fy);
            
            uint8_t r = (result >> 16) & 0xFF;
            uint8_t g = (result >> 8) & 0xFF;
            uint8_t b = result & 0xFF;
            uint8_t a = (result >> 24) & 0xFF;
            
            uint8_t exp_r = (test.expected >> 16) & 0xFF;
            uint8_t exp_g = (test.expected >> 8) & 0xFF;
            uint8_t exp_b = test.expected & 0xFF;
            uint8_t exp_a = (test.expected >> 24) & 0xFF;
            
            std::cout << "  Result: RGBA(" << (int)r << "," << (int)g << "," << (int)b << "," << (int)a << ")" << std::endl;
            std::cout << "  Expected: RGBA(" << (int)exp_r << "," << (int)exp_g << "," << (int)exp_b << "," << (int)exp_a << ")" << std::endl;
            
            if (result != test.expected) {
                std::cout << "  *** MISMATCH ***" << std::endl;
                
                // Check if it's a small precision error (within 1 unit)
                bool close = abs((int)r - (int)exp_r) <= 1 && 
                            abs((int)g - (int)exp_g) <= 1 && 
                            abs((int)b - (int)exp_b) <= 1 && 
                            abs((int)a - (int)exp_a) <= 1;
                
                if (close) {
                    std::cout << "  (Small precision error - within 1 unit)" << std::endl;
                } else {
                    std::cout << "  (Large error - potential bug!)" << std::endl;
                }
            } else {
                std::cout << "  ✓ Exact match" << std::endl;
            }
        }
        
        // Special test: What happens with all different colors at center?
        std::cout << "\nTesting four different colors at center (0.5, 0.5):" << std::endl;
        uint32_t red   = 0xFFFF0000;
        uint32_t green = 0xFF00FF00;  
        uint32_t blue  = 0xFF0000FF;
        uint32_t white = 0xFFFFFFFF;
        
        uint32_t result = test_interpolate_pixels(red, green, blue, white, 0.5, 0.5);
        
        uint8_t r = (result >> 16) & 0xFF;
        uint8_t g = (result >> 8) & 0xFF;
        uint8_t b = result & 0xFF;
        uint8_t a = (result >> 24) & 0xFF;
        
        std::cout << "  Result: RGBA(" << (int)r << "," << (int)g << "," << (int)b << "," << (int)a << ")" << std::endl;
        std::cout << "  Expected: RGBA(127,127,127,255) (average)" << std::endl;
        
        // Check for correct averaging
        // Red: (255+0+0+255)/4 = 127.5 -> 128
        // Green: (0+255+0+255)/4 = 127.5 -> 128  
        // Blue: (0+0+255+255)/4 = 127.5 -> 128
        // Alpha: (255+255+255+255)/4 = 255
        
        bool correct = (r == 127 || r == 128) && (g == 127 || g == 128) && (b == 127 || b == 128) && a == 255;
        
        if (!correct) {
            std::cout << "  *** AVERAGING BUG DETECTED ***" << std::endl;
        }
    }
}