#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include "../core/script/script_engine.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Grid Debug Analysis", "[grid_debug]") {
    SECTION("Debug script engine for identity transformation") {
        ScriptEngine engine;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        engine.set_audio_context(audio_data, false);
        
        // Test identity script manually for different input coordinates
        std::vector<std::pair<double, double>> test_coords = {
            {0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}, {0.5, 0.5}
        };
        
        std::cout << "Manual script engine test for identity 'x=x; y=y':" << std::endl;
        
        for (auto coord : test_coords) {
            engine.set_variable("x", coord.first);
            engine.set_variable("y", coord.second);
            
            // Execute the script
            engine.evaluate("x=x; y=y");
            
            double result_x = engine.get_variable("x");
            double result_y = engine.get_variable("y");
            
            std::cout << "Input (" << coord.first << ", " << coord.second 
                      << ") -> Output (" << result_x << ", " << result_y << ")" << std::endl;
            
            // Identity should preserve input
            REQUIRE(result_x == coord.first);
            REQUIRE(result_y == coord.second);
        }
    }
    
    SECTION("Test simple expression instead of identity") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Test with a simple transformation instead of identity
        table.generate(4, 4, 2, 2, "0.5", "0.5", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        std::cout << "\nTesting with constant 0.5 expressions:" << std::endl;
        auto coords = table.get_interpolated_coordinates(0.0, 0.0);
        std::cout << "Grid(0,0) with '0.5' expressions = (" << coords.first << ", " << coords.second << ")" << std::endl;
        
        // Should return (0.5, 0.5) for constant expressions
        REQUIRE(coords.first == 0.5);
        REQUIRE(coords.second == 0.5);
    }
    
    SECTION("Test explicit separate x and y expressions") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Use separate expressions instead of combined script
        table.generate(4, 4, 2, 2, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        std::cout << "\nTesting with separate 'x' and 'y' expressions:" << std::endl;
        
        auto coords_00 = table.get_interpolated_coordinates(0.0, 0.0);
        auto coords_11 = table.get_interpolated_coordinates(1.0, 1.0);
        
        std::cout << "Grid(0,0) = (" << coords_00.first << ", " << coords_00.second << ")" << std::endl;
        std::cout << "Grid(1,1) = (" << coords_11.first << ", " << coords_11.second << ")" << std::endl;
    }
}