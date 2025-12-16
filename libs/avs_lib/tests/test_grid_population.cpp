#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Grid Population Debug", "[grid_population]") {
    SECTION("Check if coordinate grid is actually empty or contains zeros") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        std::cout << "Before generation:" << std::endl;
        std::cout << "  Table valid: " << table.is_valid() << std::endl;
        
        // Generate with simple expressions
        table.generate(4, 4, 2, 2, "x", "y", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        std::cout << "After generation:" << std::endl;
        std::cout << "  Table valid: " << table.is_valid() << std::endl;
        
        // Try different coordinate requests to see if they all return (0,0)
        std::vector<std::pair<double, double>> test_coords = {
            {0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}, 
            {0.5, 0.5}, {0.25, 0.25}, {0.75, 0.75}
        };
        
        std::cout << "Coordinate lookup results:" << std::endl;
        for (auto coord : test_coords) {
            auto result = table.get_interpolated_coordinates(coord.first, coord.second);
            std::cout << "  (" << coord.first << ", " << coord.second << ") -> (" 
                      << result.first << ", " << result.second << ")" << std::endl;
        }
        
        REQUIRE(table.is_valid());
    }
    
    SECTION("Test with constant expressions") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Use constant expressions that should definitely work
        table.generate(2, 2, 2, 2, "0.5", "0.7", true, false,
                      audio_data, false, InterpolationMode::LINEAR);
        
        auto result = table.get_interpolated_coordinates(0.0, 0.0);
        std::cout << "Constant expressions '0.5', '0.7' result: (" 
                  << result.first << ", " << result.second << ")" << std::endl;
        
        // With constant expressions, all grid points should have the same value
        REQUIRE((result.first != 0.0 || result.second != 0.0)); // At least one should be non-zero
    }
    
    SECTION("Test different grid sizes") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        std::cout << "Testing different grid sizes:" << std::endl;
        
        std::vector<std::pair<int, int>> grid_sizes = {{1, 1}, {2, 2}, {3, 3}};
        
        for (auto size : grid_sizes) {
            table.generate(4, 4, size.first, size.second, "x", "y", true, false,
                          audio_data, false, InterpolationMode::LINEAR);
            
            auto result = table.get_interpolated_coordinates(0.0, 0.0);
            std::cout << "  " << size.first << "x" << size.second << " grid: (" 
                      << result.first << ", " << result.second << ")" << std::endl;
        }
        
        REQUIRE(true); // Just for debug output
    }
}