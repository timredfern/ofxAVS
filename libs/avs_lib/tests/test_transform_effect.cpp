#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "effects/transform_effect.h"
#include <vector>
#include <cmath>

using namespace avs;

TEST_CASE("Transform Effect", "[transform]") {
    TransformEffect effect;
    AudioData dummy_audio = {};
    
    SECTION("Basic effect properties") {
        REQUIRE(effect.get_name() == "Transform");
        REQUIRE(effect.get_description() == "Mathematical coordinate transformation");
        REQUIRE(effect.is_enabled() == true);
    }
    
    SECTION("Default parameters") {
        auto& params = effect.parameters();
        
        REQUIRE(params.get_bool("enabled") == true);
        REQUIRE(params.get_string("x_expr") == "x");
        REQUIRE(params.get_string("y_expr") == "y");
        REQUIRE(params.get_bool("bilinear") == true);
        REQUIRE(params.get_bool("wrap") == false);
    }
    
    SECTION("Identity transformation") {
        // Default expressions should pass through unchanged
        const int w = 4, h = 4;
        std::vector<uint32_t> input = {
            0xFF000000, 0xFF333333, 0xFF666666, 0xFF999999,
            0xFFCCCCCC, 0xFFFFFFFF, 0xFF000000, 0xFF333333,
            0xFF666666, 0xFF999999, 0xFFCCCCCC, 0xFFFFFFFF,
            0xFF000000, 0xFF333333, 0xFF666666, 0xFF999999
        };
        std::vector<uint32_t> output(w * h, 0);
        
        // Use nearest neighbor for exact comparison
        effect.parameters().set_bool("bilinear", false);
        
        int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
        
        REQUIRE(result == 1); // Uses output buffer
        
        // With identity transformation and nearest neighbor, output should exactly match input
        for (int i = 0; i < w * h; i++) {
            REQUIRE(output[i] == input[i]);
        }
    }
    
    SECTION("Simple coordinate transformations") {
        const int w = 4, h = 4;
        std::vector<uint32_t> input = {
            0xFF000000, 0xFF111111, 0xFF222222, 0xFF333333,  // y=0
            0xFF444444, 0xFF555555, 0xFF666666, 0xFF777777,  // y=1  
            0xFF888888, 0xFF999999, 0xFFAAAAAA, 0xFFBBBBBB,  // y=2
            0xFFCCCCCC, 0xFFDDDDDD, 0xFFEEEEEE, 0xFFFFFFFF   // y=3
        };
        std::vector<uint32_t> output(w * h, 0);
        
        SECTION("Horizontal flip") {
            effect.parameters().set_bool("bilinear", false);  // Exact sampling
            effect.parameters().set_string("x_expr", "1.0 - x");
            effect.parameters().set_string("y_expr", "y");
            
            int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
            REQUIRE(result == 1);
            
            // Check that first row is horizontally flipped
            REQUIRE(output[0] == input[3]);   // First pixel should be last pixel of input
            REQUIRE(output[3] == input[0]);   // Last pixel should be first pixel of input
        }
        
        SECTION("Vertical flip") {
            effect.parameters().set_bool("bilinear", false);  // Exact sampling
            effect.parameters().set_string("x_expr", "x");
            effect.parameters().set_string("y_expr", "1.0 - y");
            
            int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
            REQUIRE(result == 1);
            
            // Check that columns are vertically flipped
            REQUIRE(output[0] == input[12]);  // Top-left should be bottom-left
            REQUIRE(output[12] == input[0]);  // Bottom-left should be top-left
        }
    }
    
    SECTION("Mathematical transformations") {
        const int w = 8, h = 8;
        std::vector<uint32_t> input(w * h, 0xFF808080);  // Gray image
        std::vector<uint32_t> output(w * h, 0);
        
        SECTION("Scaling transformation") {
            // Scale by 0.5 (zoom in to center)
            effect.parameters().set_string("x_expr", "x * 0.5 + 0.25");
            effect.parameters().set_string("y_expr", "y * 0.5 + 0.25");
            
            int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
            REQUIRE(result == 1);
            
            // With scaling, the center should contain the original image content
            // Count non-black pixels (anything not 0xFF000000)
            int content_count = 0;
            for (int i = 0; i < w * h; i++) {
                if (output[i] != 0xFF000000) content_count++;
            }
            REQUIRE(content_count > 0); // Should have some content
        }
        
        SECTION("Rotation transformation") {
            // 90-degree rotation: new_x = y, new_y = 1-x
            effect.parameters().set_string("x_expr", "y");
            effect.parameters().set_string("y_expr", "1.0 - x");
            
            int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
            REQUIRE(result == 1);
            
            // Should still contain gray pixels (rotated), though sampling may affect exact count
            int gray_count = 0;
            for (int i = 0; i < w * h; i++) {
                uint32_t pixel = output[i];
                int r = (pixel >> 16) & 0xFF;
                int g = (pixel >> 8) & 0xFF;
                int b = pixel & 0xFF;
                // Allow for slight variations due to sampling
                if (r >= 0x70 && r <= 0x90 && g >= 0x70 && g <= 0x90 && b >= 0x70 && b <= 0x90) {
                    gray_count++;
                }
            }
            REQUIRE(gray_count > w * h / 4); // At least 1/4 should be grayish
        }
    }
    
    SECTION("Trigonometric transformations") {
        const int w = 6, h = 6;
        std::vector<uint32_t> input(w * h, 0xFFFFFFFF);  // White image
        std::vector<uint32_t> output(w * h, 0);
        
        SECTION("Wave distortion") {
            // Simple wave effect
            effect.parameters().set_string("x_expr", "x + 0.1 * sin(y * 6.28)");
            effect.parameters().set_string("y_expr", "y");
            
            int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
            REQUIRE(result == 1);
            
            // Should produce a wave-distorted image
            // Since we're using bilinear interpolation, most pixels should still be white or close
            int white_count = 0;
            for (int i = 0; i < w * h; i++) {
                // Count pixels that are at least mostly white
                uint32_t pixel = output[i];
                int r = (pixel >> 16) & 0xFF;
                int g = (pixel >> 8) & 0xFF;
                int b = pixel & 0xFF;
                if (r > 200 && g > 200 && b > 200) white_count++;
            }
            REQUIRE(white_count > w * h / 3); // Most should be whitish
        }
        
        SECTION("Polar coordinate conversion") {
            // Convert to polar: x becomes angle, y becomes radius
            effect.parameters().set_string("x_expr", "0.5 + 0.3 * cos(x * 6.28) * y");
            effect.parameters().set_string("y_expr", "0.5 + 0.3 * sin(x * 6.28) * y");
            
            int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
            REQUIRE(result == 1);
            
            // Should create a circular distortion pattern
            // The exact result depends on sampling, but should not be all black
            int non_black_count = 0;
            for (int i = 0; i < w * h; i++) {
                if (output[i] != 0) non_black_count++;
            }
            REQUIRE(non_black_count > 0);
        }
    }
    
    SECTION("Wrapping and clamping") {
        const int w = 4, h = 4;
        std::vector<uint32_t> input = {
            0xFF000000, 0xFF111111, 0xFF222222, 0xFF333333,
            0xFF444444, 0xFF555555, 0xFF666666, 0xFF777777,
            0xFF888888, 0xFF999999, 0xFFAAAAAA, 0xFFBBBBBB,
            0xFFCCCCCC, 0xFFDDDDDD, 0xFFEEEEEE, 0xFFFFFFFF
        };
        std::vector<uint32_t> output(w * h, 0);
        
        SECTION("Out of bounds with wrapping") {
            effect.parameters().set_bool("wrap", true);
            effect.parameters().set_string("x_expr", "x + 0.5"); // Shift by half width
            effect.parameters().set_string("y_expr", "y");
            
            int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
            REQUIRE(result == 1);
            
            // With wrapping, coordinates > 1.0 should wrap around
            // This should create a shifted version of the input
            REQUIRE(output[0] != 0); // Should not be black (clamped)
        }
        
        SECTION("Out of bounds without wrapping") {
            effect.parameters().set_bool("wrap", false);
            effect.parameters().set_bool("bilinear", false);  // Exact sampling
            effect.parameters().set_string("x_expr", "x + 0.5"); // Shift right by half
            effect.parameters().set_string("y_expr", "y");
            
            int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
            REQUIRE(result == 1);
            
            // Without wrapping, coordinates > 1.0 should be clamped to 1.0
            // This should create a shifted version that samples from the right edge
            int edge_color_count = 0;
            uint32_t expected_edge_color = input[3]; // Right edge of first row
            for (int i = 0; i < w; i++) { // Check first row
                if (output[i] == expected_edge_color) edge_color_count++;
            }
            REQUIRE(edge_color_count > 0);
        }
    }
    
    SECTION("Expression error handling") {
        const int w = 4, h = 4;
        std::vector<uint32_t> input(w * h, 0xFF808080);
        std::vector<uint32_t> output(w * h, 0);
        
        SECTION("Invalid expression") {
            effect.parameters().set_string("x_expr", "invalid_function(x)");
            effect.parameters().set_string("y_expr", "y");
            
            int result = effect.render(dummy_audio, 0, input.data(), output.data(), w, h);
            REQUIRE(result == 1);
            
            // With invalid expressions, should fall back to copying input
            for (int i = 0; i < w * h; i++) {
                REQUIRE(output[i] == input[i]);
            }
        }
    }
}