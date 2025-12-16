#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using Catch::Approx;
using namespace avs;

TEST_CASE("Coordinate Precision Test", "[coord_precision]") {
    SECTION("100x100 image with 2x2 grid - pixel 50,50 should map to itself") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        int width = 100, height = 100;
        int grid_width = 2, grid_height = 2;
        
        // Generate identity grid with linear interpolation
        table.generate(width, height, grid_width, grid_height, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        // Calculate what grid coordinates pixel (50,50) maps to
        double grid_x = (50.0 * (grid_width - 1.0)) / (width - 1.0);
        double grid_y = (50.0 * (grid_height - 1.0)) / (height - 1.0);
        
        std::cout << "Pixel (50,50) maps to grid coordinates: (" << grid_x << ", " << grid_y << ")" << std::endl;
        
        // Get the interpolated coordinates at that grid position
        auto interpolated = table.get_interpolated_coordinates(grid_x, grid_y);
        std::cout << "Grid returns normalized coordinates: (" << interpolated.first << ", " << interpolated.second << ")" << std::endl;
        
        // Convert back to pixel coordinates
        double src_x = interpolated.first * (width - 1);
        double src_y = interpolated.second * (height - 1);
        
        std::cout << "Which converts back to pixel coordinates: (" << src_x << ", " << src_y << ")" << std::endl;
        
        // For identity transformation, pixel (50,50) should map back to approximately (50,50)
        REQUIRE(src_x == Approx(50.0).margin(0.1));
        REQUIRE(src_y == Approx(50.0).margin(0.1));
    }
    
    SECTION("Check grid corner coordinates for 2x2 identity grid") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Simple case: 2x2 grid
        table.generate(100, 100, 2, 2, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        // Check the four grid corner coordinates through interpolation
        std::cout << "\nGrid corner values for 2x2 identity grid:" << std::endl;
        
        auto grid_00 = table.get_interpolated_coordinates(0.0, 0.0);
        auto grid_10 = table.get_interpolated_coordinates(1.0, 0.0);
        auto grid_01 = table.get_interpolated_coordinates(0.0, 1.0);
        auto grid_11 = table.get_interpolated_coordinates(1.0, 1.0);
        
        std::cout << "Grid(0,0) = (" << grid_00.first << ", " << grid_00.second << ")" << std::endl;
        std::cout << "Grid(1,0) = (" << grid_10.first << ", " << grid_10.second << ")" << std::endl;
        std::cout << "Grid(0,1) = (" << grid_01.first << ", " << grid_01.second << ")" << std::endl;
        std::cout << "Grid(1,1) = (" << grid_11.first << ", " << grid_11.second << ")" << std::endl;
        
        // For identity transformation, the grid should contain:
        // (0,0) -> (0,0), (1,0) -> (1,0), (0,1) -> (0,1), (1,1) -> (1,1)
        REQUIRE(grid_00.first == Approx(0.0).margin(0.01));
        REQUIRE(grid_00.second == Approx(0.0).margin(0.01));
        REQUIRE(grid_10.first == Approx(1.0).margin(0.01));
        REQUIRE(grid_10.second == Approx(0.0).margin(0.01));
        REQUIRE(grid_01.first == Approx(0.0).margin(0.01));
        REQUIRE(grid_01.second == Approx(1.0).margin(0.01));
        REQUIRE(grid_11.first == Approx(1.0).margin(0.01));
        REQUIRE(grid_11.second == Approx(1.0).margin(0.01));
    }
    
    SECTION("Test center interpolation with exact calculation") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        table.generate(100, 100, 2, 2, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        // Center of image should be at grid coordinates (0.5, 0.5) 
        // and should interpolate to normalized (0.5, 0.5)
        auto center_coords = table.get_interpolated_coordinates(0.5, 0.5);
        std::cout << "\nCenter interpolation (0.5, 0.5) gives: (" 
                  << center_coords.first << ", " << center_coords.second << ")" << std::endl;
        
        // Center should map to center for identity
        REQUIRE(center_coords.first == Approx(0.5).margin(0.01));
        REQUIRE(center_coords.second == Approx(0.5).margin(0.01));
    }
}