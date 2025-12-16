#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../core/coordinate_lookup_table.h"

using namespace avs;

TEST_CASE("Coordinate Interpolation Grid", "[coordinate][interpolation]") {
    
    SECTION("Grid-based coordinate transformation") {
        // Test the core concept: grid evaluation with coordinate interpolation
        // This is what DynamicMovement uses vs MovementEffect's full-resolution approach
        
        const int output_width = 64;
        const int output_height = 64;
        const int grid_width = 9;   // Low resolution grid with clear center
        const int grid_height = 9;
        
        CoordinateLookupTable grid;
        AudioData dummy_audio = {};
        
        // Identity transformation: x_out = x_in, y_out = y_in
        grid.generate(output_width, output_height, grid_width, grid_height, 
                     "x", "y", true, false, dummy_audio, false, InterpolationMode::NONE);
        
        SECTION("Grid point access") {
            // At grid points, coordinates should be exact
            // Grid point (4,4) represents center of 9x9 grid
            std::pair<double, double> coords = grid.get_interpolated_coordinates(4, 4);
            
            // Center of 9x9 grid: 4/(9-1) = 4/8 = 0.5 exactly
            REQUIRE(coords.first == 0.5);   // x = center (exact)
            REQUIRE(coords.second == 0.5);  // y = center (exact)
        }
        
        SECTION("Nearest neighbor interpolation") {
            grid.set_interpolation_mode(InterpolationMode::NONE);
            
            // Point between grid cells should use nearest grid point (quantized/stepped effect)
            std::pair<double, double> coords = grid.get_interpolated_coordinates(3.7, 3.2);
            std::pair<double, double> expected = grid.get_interpolated_coordinates(4, 3);  // Nearest grid point
            
            REQUIRE(coords.first == Catch::Approx(expected.first).epsilon(0.001));
            REQUIRE(coords.second == Catch::Approx(expected.second).epsilon(0.001));
        }
        
        SECTION("Bilinear interpolation") {
            grid.set_interpolation_mode(InterpolationMode::LINEAR);
            
            // Point exactly between four grid cells should be average of them
            std::pair<double, double> coords = grid.get_interpolated_coordinates(3.5, 3.5);
            
            // Get the four surrounding grid points
            auto tl = grid.get_interpolated_coordinates(3, 3);    // top-left
            auto tr = grid.get_interpolated_coordinates(4, 3);    // top-right  
            auto bl = grid.get_interpolated_coordinates(3, 4);    // bottom-left
            auto br = grid.get_interpolated_coordinates(4, 4);    // bottom-right
            
            // Bilinear interpolation at (0.5, 0.5) should be average of four corners
            double expected_x = (tl.first + tr.first + bl.first + br.first) / 4.0;
            double expected_y = (tl.second + tr.second + bl.second + br.second) / 4.0;
            
            REQUIRE(coords.first == Catch::Approx(expected_x).epsilon(0.01));
            REQUIRE(coords.second == Catch::Approx(expected_y).epsilon(0.01));
        }
    }
    
    SECTION("Script-based transformation") {
        const int output_width = 32;
        const int output_height = 32;
        const int grid_width = 5;   // 5x5 grid has clear center at (2,2)
        const int grid_height = 5;
        
        CoordinateLookupTable grid;
        AudioData dummy_audio = {};
        
        // Simple transformation: rotate 90 degrees (x_out = y, y_out = 1-x)
        grid.generate(output_width, output_height, grid_width, grid_height,
                     "y", "1.0 - x", true, false, dummy_audio, false, InterpolationMode::NONE);
        
        SECTION("90 degree rotation verification") {
            // Top-left input (0,0): norm coords (0,0) -> x_out = y = 0, y_out = 1-x = 1-0 = 1
            auto coords = grid.get_interpolated_coordinates(0, 0);
            REQUIRE(coords.first == 0.0);   // y = 0 (exact)
            REQUIRE(coords.second == 1.0);  // 1 - x = 1 - 0 = 1 (exact)
            
            // Top-right input (4,0): norm coords (1,0) -> x_out = y = 0, y_out = 1-x = 1-1 = 0 
            coords = grid.get_interpolated_coordinates(grid_width-1, 0);
            REQUIRE(coords.first == 0.0);   // y = 0 (exact)
            REQUIRE(coords.second == 0.0);  // 1 - x = 1 - 1 = 0 (exact)
        }
    }
    
    SECTION("Classical AVS swirl transformation") {
        const int grid_size = 9;  // 9x9 grid with clear center
        CoordinateLookupTable grid;
        AudioData dummy_audio = {};
        
        // Classic swirl: convert to polar, modify angle, convert back
        // This is similar to "big swirl out" preset
        std::string swirl_script = 
            "r_coord = atan2(y - 0.5, x - 0.5); "
            "d_coord = sqrt((x - 0.5) * (x - 0.5) + (y - 0.5) * (y - 0.5)); "
            "r_coord = r_coord + 0.1; "
            "d_coord = d_coord * 0.96; "
            "x_out = 0.5 + cos(r_coord) * d_coord; "
            "y_out = 0.5 + sin(r_coord) * d_coord; "
            "x_out";
        
        std::string swirl_script_y =
            "r_coord = atan2(y - 0.5, x - 0.5); "
            "d_coord = sqrt((x - 0.5) * (x - 0.5) + (y - 0.5) * (y - 0.5)); "
            "r_coord = r_coord + 0.1; "
            "d_coord = d_coord * 0.96; "
            "x_out = 0.5 + cos(r_coord) * d_coord; "
            "y_out = 0.5 + sin(r_coord) * d_coord; "
            "y_out";
            
        grid.generate(32, 32, grid_size, grid_size, swirl_script, swirl_script_y, 
                     true, false, dummy_audio, false, InterpolationMode::LINEAR);
        
        SECTION("Center point should be approximately preserved") {
            auto coords = grid.get_interpolated_coordinates(grid_size/2, grid_size/2);
            // Center should be close to center after swirl (small movement)
            // For 9x9 grid, center is 4/(9-1) = 4/8 = 0.5 exactly
            REQUIRE(coords.first == Catch::Approx(0.5).epsilon(0.2));
            REQUIRE(coords.second == Catch::Approx(0.5).epsilon(0.2));
        }
        
        SECTION("Non-center points should be transformed") {
            // Off-center points should be noticeably transformed by swirl
            auto coords = grid.get_interpolated_coordinates(0, 0);
            // Should not be identity transformation 
            bool transformed = (abs(coords.first - 0.0) > 0.05) || (abs(coords.second - 0.0) > 0.05);
            REQUIRE(transformed);
        }
    }
}