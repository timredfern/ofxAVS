#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "script/script_engine.h"

using namespace avs;

TEST_CASE("AVS-specific variables", "[avs_variables]") {
    ScriptEngine engine;
    
    SECTION("Pixel coordinate variables") {
        SECTION("x and y coordinates") {
            // Set up coordinate context for a 100x100 pixel at (50, 75)
            engine.set_pixel_context(50, 75, 100, 100);
            
            double x_result = engine.evaluate("x");
            double y_result = engine.evaluate("y");
            
            REQUIRE(x_result == Catch::Approx(50.0/99.0));  // 50/99 (normalized with width-1)
            REQUIRE(y_result == Catch::Approx(75.0/99.0)); // 75/99 (normalized with height-1)
        }
        
        SECTION("w and h dimensions") {
            engine.set_pixel_context(25, 50, 200, 400);
            
            double w_result = engine.evaluate("w");
            double h_result = engine.evaluate("h");
            
            REQUIRE(w_result == Catch::Approx(200.0));
            REQUIRE(h_result == Catch::Approx(400.0));
        }
    }
    
    SECTION("Color channel variables") {
        SECTION("r, g, b color values") {
            // Set RGB color (0.8, 0.6, 0.2) for current pixel
            engine.set_color_context(0.8, 0.6, 0.2);
            
            double r_result = engine.evaluate("r");
            double g_result = engine.evaluate("g");
            double b_result = engine.evaluate("b");
            
            REQUIRE(r_result == Catch::Approx(0.8));
            REQUIRE(g_result == Catch::Approx(0.6));
            REQUIRE(b_result == Catch::Approx(0.2));
        }
        
        SECTION("Color manipulation") {
            engine.set_color_context(0.5, 0.7, 0.3);
            
            // Test expressions that modify colors
            double enhanced_red = engine.evaluate("r * 1.2");
            double dimmed_green = engine.evaluate("g * 0.8");
            double inverted_blue = engine.evaluate("1.0 - b");
            
            REQUIRE(enhanced_red == Catch::Approx(0.6));
            REQUIRE(dimmed_green == Catch::Approx(0.56));
            REQUIRE(inverted_blue == Catch::Approx(0.7));
        }
    }
    
    SECTION("Combined coordinate and color expressions") {
        SECTION("Position-based color effects") {
            engine.set_pixel_context(30, 70, 100, 100);
            engine.set_color_context(0.5, 0.5, 0.5);
            
            // Gradient effect: red increases with x position  
            double expected_x = 30.0 / 99.0; // 30 / (100-1)
            double gradient_red = engine.evaluate("x * r");
            REQUIRE(gradient_red == Catch::Approx(expected_x * 0.5)); // (30/99) * 0.5
            
            // Distance-based effect using coordinates
            double expected_y = 70.0 / 99.0; // 70 / (100-1)
            double distance_factor = engine.evaluate("sqrt(x*x + y*y)");
            REQUIRE(distance_factor == Catch::Approx(std::sqrt(expected_x*expected_x + expected_y*expected_y)).epsilon(0.001));
        }
        
        SECTION("Complex color transformations") {
            engine.set_pixel_context(50, 50, 100, 100);
            engine.set_color_context(0.8, 0.4, 0.6);
            
            // HSV-like transformation using coordinates
            double hue_shift = engine.evaluate("sin(x * 3.14159) * 0.5 + 0.5");
            double saturation = engine.evaluate("sqrt(r*r + g*g + b*b) / sqrt(3.0)");
            
            REQUIRE(hue_shift == Catch::Approx(1.0).epsilon(0.001)); // sin(π/2) * 0.5 + 0.5
            REQUIRE(saturation == Catch::Approx(0.622).epsilon(0.01)); // sqrt(0.8^2+0.4^2+0.6^2)/sqrt(3)
        }
    }
    
    SECTION("Manual variable updates") {
        SECTION("Update color context and verify") {
            engine.set_color_context(0.2, 0.4, 0.6);
            
            // Verify initial values
            REQUIRE(engine.get_variable("r") == Catch::Approx(0.2));
            REQUIRE(engine.get_variable("g") == Catch::Approx(0.4));
            REQUIRE(engine.get_variable("b") == Catch::Approx(0.6));
            
            // Update context and verify new values
            engine.set_color_context(0.9, 0.3, 0.5);
            
            REQUIRE(engine.get_variable("r") == Catch::Approx(0.9));
            REQUIRE(engine.get_variable("g") == Catch::Approx(0.3));
            REQUIRE(engine.get_variable("b") == Catch::Approx(0.5));
        }
        
        SECTION("Update pixel context and verify") {
            engine.set_pixel_context(25, 75, 100, 100);
            
            // Verify initial values
            REQUIRE(engine.get_variable("x") == Catch::Approx(25.0/99.0)); // 25/(100-1)
            REQUIRE(engine.get_variable("y") == Catch::Approx(75.0/99.0)); // 75/(100-1)
            
            // Update context and verify new values
            engine.set_pixel_context(10, 20, 100, 100);
            
            REQUIRE(engine.get_variable("x") == Catch::Approx(10.0/99.0)); // 10/(100-1)
            REQUIRE(engine.get_variable("y") == Catch::Approx(20.0/99.0)); // 20/(100-1)
        }
    }
}