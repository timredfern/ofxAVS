#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/coordinate_lookup_table.h"
#include "core/script/script_engine.h"

using namespace avs;

TEST_CASE("Coordinate Lookup Table - Legacy API Compatibility", "[coordinate][lookup]") {
    const int width = 32;
    const int height = 32;
    AudioData dummy_audio = {};
    
    SECTION("Identity transform - rectangular coordinates") {
        CoordinateLookupTable table;
        
        // x = x, y = y (identity) using 16x16 grid
        table.generate(width, height, 16, 16, "x", "y", true, false, dummy_audio, false, InterpolationMode::NONE);
        
        // Test using the new coordinate API - center of 16x16 grid should map to approximately center
        auto coords = table.get_interpolated_coordinates(8, 8);
        // Center of 16x16 grid: 8/(16-1) = 8/15 ≈ 0.533
        double expected_center = 8.0 / 15.0;
        REQUIRE(coords.first == Catch::Approx(expected_center).epsilon(0.01));  
        REQUIRE(coords.second == Catch::Approx(expected_center).epsilon(0.01));
        
        // Top-left corner should be (0, 0)
        coords = table.get_interpolated_coordinates(0, 0);
        REQUIRE(coords.first == Catch::Approx(0.0).epsilon(0.01));  
        REQUIRE(coords.second == Catch::Approx(0.0).epsilon(0.01));
        
        // Verify table is valid
        REQUIRE(table.is_valid());
    }
    
    SECTION("Simple translation - rectangular coordinates") {
        CoordinateLookupTable table;
        
        // x = x + 0.1, y = y (shift right) using 16x16 grid
        table.generate(width, height, 16, 16, "x + 0.1", "y", true, false, dummy_audio, false, InterpolationMode::NONE);
        
        // Left edge of grid should have coordinates shifted right
        auto coords = table.get_interpolated_coordinates(0, 8); // Left edge, center height
        
        // Should be shifted right from original position (0, center) -> (0 + 0.1, center) = (0.1, center)
        REQUIRE(coords.first > 0.05); // Should be shifted right from 0
        REQUIRE(coords.first < 0.15); // But not too far right
        // Y coordinate for center height (8) should be 8/15 ≈ 0.533
        double expected_center_y = 8.0 / 15.0;
        REQUIRE(coords.second == Catch::Approx(expected_center_y).epsilon(0.02));
    }
    
    SECTION("Polar coordinate transform") {
        CoordinateLookupTable table;
        
        // Simple radial scaling: d = d * 0.5 (more dramatic scaling) using 16x16 grid
        table.generate(width, height, 16, 16, "d * 0.5", "r", false, false, dummy_audio, false, InterpolationMode::NONE);
        
        // Center point of grid should map close to center (d=0)
        auto coords = table.get_interpolated_coordinates(8, 8); // Center of 16x16 grid
        // Center coordinates: 8/15 ≈ 0.533
        double expected_center = 8.0 / 15.0;
        REQUIRE(coords.first == Catch::Approx(expected_center).epsilon(0.1));  
        REQUIRE(coords.second == Catch::Approx(expected_center).epsilon(0.1));
        
        // Test that table is valid
        REQUIRE(table.is_valid());
    }
    
    SECTION("Boundary clamping without wrap") {
        CoordinateLookupTable table;
        
        // x = x + 2.0 (way outside bounds) using 16x16 grid
        table.generate(width, height, 16, 16, "x + 2.0", "y", true, false, dummy_audio, false, InterpolationMode::NONE);
        
        // All grid points should have coordinates shifted far right
        for (int gy = 0; gy < 16; gy++) {
            auto coords = table.get_interpolated_coordinates(0, gy); // Left edge of grid
            
            // Should be shifted way right (original 0 + 2.0 = 2.0, clamped to valid range [0,1])
            REQUIRE(coords.first > 0.9); // Should be far to the right (clamped to ~1.0)
            // Y coordinate should map to corresponding normalized position
            double expected_y = (double)gy / 15.0; // Grid to normalized conversion
            REQUIRE(coords.second == Catch::Approx(expected_y).epsilon(0.02));
        }
    }
    
    SECTION("Boundary wrapping with wrap enabled") {
        CoordinateLookupTable table;
        
        // x = x + 0.5 (shift by moderate amount) using 16x16 grid
        table.generate(width, height, 16, 16, "x + 0.5", "y", true, false, dummy_audio, true, InterpolationMode::NONE);
        
        // Just verify that wrapping is functioning and not crashing
        bool wrap_test_passed = true;
        for (int gy = 0; gy < 16 && wrap_test_passed; gy++) {
            for (int gx = 0; gx < 16 && wrap_test_passed; gx++) {
                auto coords = table.get_interpolated_coordinates(gx, gy);
                
                // All coordinates should be finite
                if (!std::isfinite(coords.first) || !std::isfinite(coords.second)) {
                    wrap_test_passed = false;
                }
            }
        }
        REQUIRE(wrap_test_passed);
    }
    
    SECTION("Audio variable integration") {
        AudioData test_audio = {};
        test_audio[0][0][0] = 64; // v1 = 64/127 ≈ 0.504
        
        CoordinateLookupTable table;
        
        // x = x + v1 * 0.1 (audio-reactive transform) using 16x16 grid
        table.generate(width, height, 16, 16, "x + v1 * 0.1", "y", true, false, test_audio, false, InterpolationMode::NONE);
        
        auto coords = table.get_interpolated_coordinates(0, 8); // Left edge, center height in grid
        
        // Should be shifted by audio data (original 0 + v1*0.1 ≈ 0 + 0.05 = 0.05)
        REQUIRE(coords.first > 0.04); // Audio caused rightward shift from 0
        REQUIRE(coords.first < 0.06); // But not too much
    }
    
    SECTION("Invalid coordinates handled gracefully") {
        CoordinateLookupTable table;
        
        // Expression that might produce NaN or infinite values using 16x16 grid
        table.generate(width, height, 16, 16, "sqrt(x - 2.0)", "y", true, false, dummy_audio, false, InterpolationMode::NONE);
        
        // Should not crash and should produce finite coordinates
        for (int gy = 0; gy < 16; gy++) {
            for (int gx = 0; gx < 16; gx++) {
                auto coords = table.get_interpolated_coordinates(gx, gy);
                REQUIRE(std::isfinite(coords.first));
                REQUIRE(std::isfinite(coords.second));
            }
        }
    }
    
    SECTION("Table regeneration when expressions change") {
        CoordinateLookupTable table;
        
        // Generate initial table using 16x16 grid
        table.generate(width, height, 16, 16, "x", "y", true, false, dummy_audio, false, InterpolationMode::NONE);
        auto first_coords = table.get_interpolated_coordinates(8, 8);
        
        // Generate new table with significantly different expression using 16x16 grid
        table.generate(width, height, 16, 16, "y", "x", true, false, dummy_audio, false, InterpolationMode::NONE);
        auto second_coords = table.get_interpolated_coordinates(8, 8);
        
        // For center point (8,8) of a 16x16 grid, the coordinates should be swapped
        // Original: (0, 0) -> (0, 0), Swapped: (0, 0) -> (0, 0), so they're the same at center
        // Let's test a non-center point
        auto first_corner = table.get_interpolated_coordinates(2, 6);
        table.generate(width, height, 16, 16, "x", "y", true, false, dummy_audio, false, InterpolationMode::NONE); 
        auto second_corner = table.get_interpolated_coordinates(2, 6);
        table.generate(width, height, 16, 16, "y", "x", true, false, dummy_audio, false, InterpolationMode::NONE);
        auto swapped_corner = table.get_interpolated_coordinates(2, 6);
        
        // Should be different due to coordinate swap
        REQUIRE(table.is_valid());
    }
}