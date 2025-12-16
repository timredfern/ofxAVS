#include <catch2/catch_test_macros.hpp>
#include "../core/coordinate_lookup_table.h"
#include "../core/script/script_engine.h"
#include <cstring>
#include <iostream>

using namespace avs;

TEST_CASE("Script Execution Trace", "[script_trace]") {
    SECTION("Manually trace script execution for grid generation") {
        ScriptEngine engine;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        engine.set_audio_context(audio_data, false);
        
        // Simulate what happens in generate_rectangular for a 2x2 grid
        int grid_width = 2, grid_height = 2;
        int output_width = 4, output_height = 4;
        
        std::cout << "Manual trace of grid generation for 2x2 grid:" << std::endl;
        
        for (int gy = 0; gy < grid_height; gy++) {
            for (int gx = 0; gx < grid_width; gx++) {
                std::cout << "\n--- Grid point (" << gx << ", " << gy << ") ---" << std::endl;
                
                // Convert grid coordinates to normalized coordinates [0, 1]
                double norm_x = (double)gx / (grid_width - 1);
                double norm_y = (double)gy / (grid_height - 1);
                std::cout << "Normalized coords: (" << norm_x << ", " << norm_y << ")" << std::endl;
                
                // Set pixel context (convert grid to pixel coordinates for context)
                int pixel_x = (gx * output_width) / grid_width;
                int pixel_y = (gy * output_height) / grid_height;
                engine.set_pixel_context(pixel_x, pixel_y, output_width, output_height);
                std::cout << "Pixel context: (" << pixel_x << ", " << pixel_y << ")" << std::endl;
                
                // Set normalized coordinates as variables
                engine.set_variable("x", norm_x);
                engine.set_variable("y", norm_y);
                std::cout << "Set engine variables: x=" << norm_x << ", y=" << norm_y << std::endl;
                
                // Test different expressions
                std::vector<std::pair<std::string, std::string>> test_cases = {
                    {"x", "y"},           // Simple variable lookup
                    {"0.5", "0.7"},      // Constants
                };
                
                for (auto& test_case : test_cases) {
                    std::cout << "  Testing expressions: x='" << test_case.first 
                              << "' y='" << test_case.second << "'" << std::endl;
                    
                    // Reset variables before each test
                    engine.set_variable("x", norm_x);
                    engine.set_variable("y", norm_y);
                    
                    if (test_case.first == test_case.second) {
                        // Multi-statement
                        engine.evaluate(test_case.first);
                        double dest_norm_x = engine.get_variable("x");
                        double dest_norm_y = engine.get_variable("y");
                        std::cout << "    Multi-statement result: (" << dest_norm_x << ", " << dest_norm_y << ")" << std::endl;
                    } else {
                        // Separate expressions
                        double dest_norm_x = engine.evaluate(test_case.first);
                        double dest_norm_y = engine.evaluate(test_case.second);
                        std::cout << "    Separate result: (" << dest_norm_x << ", " << dest_norm_y << ")" << std::endl;
                    }
                }
            }
        }
        
        REQUIRE(true); // Just for tracing
    }
}