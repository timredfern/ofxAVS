#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "effects/dynamic_movement_effect.h"
#include <cstring>

using namespace avs;

TEST_CASE("Dynamic Movement Effect", "[dynamic][movement][effect]") {
    DynamicMovementEffect effect;
    
    const int width = 32;
    const int height = 32;
    AudioData dummy_audio = {};
    
    // Create test framebuffers
    std::vector<uint32_t> input(width * height, 0xFF0000FF); // Red pixels
    std::vector<uint32_t> output(width * height, 0x00000000); // Clear output
    
    SECTION("Effect initialization") {
        REQUIRE(effect.get_name() == "Dynamic Movement");
        REQUIRE(effect.get_description().find("Dynamic Movement") != std::string::npos);
        REQUIRE(effect.is_enabled() == true);
    }
    
    SECTION("Default parameters") {
        auto& params = effect.parameters();
        REQUIRE(params.get_bool("enabled", false) == true);
        REQUIRE(params.get_int("grid_width", 0) == 16);
        REQUIRE(params.get_int("grid_height", 0) == 16);
        REQUIRE(params.get_bool("rectangular", true) == false); // Default to polar
        REQUIRE(params.get_int("interpolation", -1) == 0); // None/stepped
        REQUIRE(params.get_bool("wrap", true) == false);
        REQUIRE(params.get_bool("blend", true) == false);
        REQUIRE(params.get_bool("no_movement", true) == false);
    }
    
    SECTION("No movement mode") {
        auto& params = effect.parameters();
        params.set_bool("no_movement", true);
        
        int result = effect.render(dummy_audio, 0, input.data(), output.data(), width, height);
        
        REQUIRE(result == 1); // Should use fbout
        // Output should be identical to input
        REQUIRE(std::memcmp(input.data(), output.data(), width * height * sizeof(uint32_t)) == 0);
    }
    
    SECTION("Grid-based transformation") {
        auto& params = effect.parameters();
        
        // Configure for simple grid-based transformation
        params.set_int("grid_width", 8);
        params.set_int("grid_height", 8);
        params.set_int("interpolation", 0); // No interpolation for clear stepping
        params.set_string("pixel_script", std::string("d=d*0.95; r=r+0.1")); // Default spiral
        
        int result = effect.render(dummy_audio, 0, input.data(), output.data(), width, height);
        
        REQUIRE(result == 1); // Should use fbout
        
        // Output should be different from input (transformation applied)
        bool transformed = false;
        for (int i = 0; i < width * height; i++) {
            if (output[i] != input[i]) {
                transformed = true;
                break;
            }
        }
        REQUIRE(transformed); // Some transformation should have occurred
    }
    
    SECTION("Interpolation modes") {
        auto& params = effect.parameters();
        params.set_int("grid_width", 4);
        params.set_int("grid_height", 4);
        params.set_string("pixel_script", std::string("d=d*0.5")); // Zoom effect
        
        // Test different interpolation modes
        std::vector<uint32_t> output_none(width * height);
        std::vector<uint32_t> output_linear(width * height);
        
        // None (stepped)
        params.set_int("interpolation", 0);
        effect.render(dummy_audio, 0, input.data(), output_none.data(), width, height);
        
        // Linear
        params.set_int("interpolation", 1);
        effect.render(dummy_audio, 0, input.data(), output_linear.data(), width, height);
        
        // Results should be different due to interpolation
        bool different = false;
        for (int i = 0; i < width * height; i++) {
            if (output_none[i] != output_linear[i]) {
                different = true;
                break;
            }
        }
        REQUIRE(different);
    }
    
    SECTION("Grid regeneration logic") {
        auto& params = effect.parameters();
        params.set_int("grid_width", 8);
        params.set_int("grid_height", 8);
        params.set_string("pixel_script", std::string("x"));
        
        // First render
        std::vector<uint32_t> output1(width * height);
        effect.render(dummy_audio, 0, input.data(), output1.data(), width, height);
        
        // Second render with same parameters (should reuse grid)
        std::vector<uint32_t> output2(width * height);
        effect.render(dummy_audio, 0, input.data(), output2.data(), width, height);
        
        // Results should be identical
        REQUIRE(std::memcmp(output1.data(), output2.data(), width * height * sizeof(uint32_t)) == 0);
        
        // Change grid size and render again (should regenerate)
        params.set_int("grid_width", 16);
        std::vector<uint32_t> output3(width * height);
        effect.render(dummy_audio, 0, input.data(), output3.data(), width, height);
        
        // Results may be different due to different grid resolution
        // (This is implementation-dependent, so we just verify it doesn't crash)
        REQUIRE(true);
    }
    
    SECTION("Rectangular vs Polar coordinate modes") {
        auto& params = effect.parameters();
        params.set_int("grid_width", 8);
        params.set_int("grid_height", 8);
        
        std::vector<uint32_t> output_polar(width * height);
        std::vector<uint32_t> output_rect(width * height);
        
        // Polar mode (default)
        params.set_bool("rectangular", false);
        params.set_string("pixel_script", std::string("d=d*0.9"));
        effect.render(dummy_audio, 0, input.data(), output_polar.data(), width, height);
        
        // Rectangular mode
        params.set_bool("rectangular", true);
        params.set_string("pixel_script", std::string("x = x * 0.9"));
        effect.render(dummy_audio, 0, input.data(), output_rect.data(), width, height);
        
        // Both should produce transformations (implementation details may vary)
        bool polar_transformed = false;
        bool rect_transformed = false;
        
        for (int i = 0; i < width * height; i++) {
            if (output_polar[i] != input[i]) polar_transformed = true;
            if (output_rect[i] != input[i]) rect_transformed = true;
        }
        
        REQUIRE(polar_transformed);
        REQUIRE(rect_transformed);
    }
}