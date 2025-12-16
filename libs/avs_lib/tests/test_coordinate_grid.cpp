#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../core/coordinate_lookup_table.h"
#include "../core/script/script_engine.h"
#include <cstring>
#include <iostream>
#include <iomanip>

using Catch::Approx;

using namespace avs;

// Helper function to create a test pattern
void create_test_pattern(uint32_t* buffer, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Create a gradient pattern with unique value for each pixel
            int r = (x * 255) / (width - 1);
            int g = (y * 255) / (height - 1);
            int b = ((x + y) * 255) / (width + height - 2);
            buffer[y * width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

// Helper to check if two buffers are identical
bool buffers_equal(const uint32_t* buf1, const uint32_t* buf2, int size) {
    for (int i = 0; i < size; i++) {
        if (buf1[i] != buf2[i]) {
            return false;
        }
    }
    return true;
}

// Helper to print buffer differences
void print_buffer_diff(const uint32_t* expected, const uint32_t* actual, int width, int height) {
    int diff_count = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            if (expected[idx] != actual[idx]) {
                if (diff_count < 10) { // Only print first 10 differences
                    std::cout << "Diff at (" << x << "," << y << "): "
                              << std::hex << "expected 0x" << expected[idx] 
                              << " got 0x" << actual[idx] << std::dec << std::endl;
                }
                diff_count++;
            }
        }
    }
    if (diff_count > 10) {
        std::cout << "... and " << (diff_count - 10) << " more differences" << std::endl;
    }
}

TEST_CASE("CoordinateLookupTable Identity Transformation", "[coordinate_grid]") {
    const int width = 32;
    const int height = 32;
    
    SECTION("Identity with x=x; y=y should preserve image") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Generate identity transformation
        table.generate(width, height, 8, 8,  // 8x8 grid
                      "x", "y",  // Identity: x=x, y=y
                      true,      // rectangular
                      false,     // no subpixel
                      audio_data, false, InterpolationMode::NONE);
        
        // Create test pattern
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height);
        create_test_pattern(input.data(), width, height);
        
        // Apply identity transformation
        table.apply(input.data(), output.data(), width, height, false);
        
        // Check if output equals input
        if (!buffers_equal(input.data(), output.data(), width * height)) {
            print_buffer_diff(input.data(), output.data(), width, height);
        }
        
        REQUIRE(buffers_equal(input.data(), output.data(), width * height));
    }
    
    SECTION("Identity with higher grid resolution") {
        CoordinateLookupTable table;
        AudioData audio_data;
        memset(audio_data, 0, sizeof(AudioData));
        
        // Generate identity with exact grid (one point per pixel)
        table.generate(width, height, width, height,
                      "x", "y",
                      true, false,
                      audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height);
        create_test_pattern(input.data(), width, height);
        
        table.apply(input.data(), output.data(), width, height, false);
        
        REQUIRE(buffers_equal(input.data(), output.data(), width * height));
    }
}

TEST_CASE("CoordinateLookupTable Basic Transformations", "[coordinate_grid]") {
    const int width = 32;
    const int height = 32;
    AudioData audio_data;
    memset(audio_data, 0, sizeof(AudioData));
    
    SECTION("Simple offset x=x+0.1 should shift image right") {
        CoordinateLookupTable table;
        
        // Shift right by 10% of image width
        table.generate(width, height, 8, 8,
                      "x+0.1", "y",
                      true, false,
                      audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0);
        
        // Create vertical stripe pattern to see horizontal shift
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                input[y * width + x] = (x < width/2) ? 0xFFFF0000 : 0xFF0000FF;
            }
        }
        
        table.apply(input.data(), output.data(), width, height, false);
        
        // Check that red stripe moved right
        int red_count_left = 0;
        int red_count_right = 0;
        for (int x = 0; x < width/2; x++) {
            if ((output[height/2 * width + x] & 0x00FF0000) == 0x00FF0000) {
                red_count_left++;
            }
        }
        for (int x = width/2; x < width; x++) {
            if ((output[height/2 * width + x] & 0x00FF0000) == 0x00FF0000) {
                red_count_right++;
            }
        }
        
        // After shift right, more red should be on the right side
        REQUIRE(red_count_right > red_count_left);
    }
    
    SECTION("Simple offset y=y+0.1 should shift image down") {
        CoordinateLookupTable table;
        
        // Shift down by 10% of image height
        table.generate(width, height, 8, 8,
                      "x", "y+0.1",
                      true, false,
                      audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0);
        
        // Create horizontal stripe pattern to see vertical shift
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                input[y * width + x] = (y < height/2) ? 0xFFFF0000 : 0xFF0000FF;
            }
        }
        
        table.apply(input.data(), output.data(), width, height, false);
        
        // Check that red stripe moved down
        int red_count_top = 0;
        int red_count_bottom = 0;
        for (int y = 0; y < height/2; y++) {
            if ((output[y * width + width/2] & 0x00FF0000) == 0x00FF0000) {
                red_count_top++;
            }
        }
        for (int y = height/2; y < height; y++) {
            if ((output[y * width + width/2] & 0x00FF0000) == 0x00FF0000) {
                red_count_bottom++;
            }
        }
        
        // After shift down, more red should be on the bottom
        REQUIRE(red_count_bottom > red_count_top);
    }
}

TEST_CASE("CoordinateLookupTable Grid Interpolation", "[coordinate_grid]") {
    const int width = 32;
    const int height = 32;
    AudioData audio_data;
    memset(audio_data, 0, sizeof(AudioData));
    
    SECTION("Linear interpolation should smooth transformations") {
        CoordinateLookupTable table_none;
        CoordinateLookupTable table_linear;
        
        // Same transformation with different interpolation modes
        table_none.generate(width, height, 4, 4,  // Low res grid
                           "x", "y+0.1",
                           true, false,
                           audio_data, false, InterpolationMode::NONE);
        
        table_linear.generate(width, height, 4, 4,  // Low res grid
                             "x", "y+0.1",
                             true, false,
                             audio_data, false, InterpolationMode::LINEAR);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output_none(width * height, 0);
        std::vector<uint32_t> output_linear(width * height, 0);
        
        create_test_pattern(input.data(), width, height);
        
        table_none.apply(input.data(), output_none.data(), width, height, false);
        table_linear.apply(input.data(), output_linear.data(), width, height, false);
        
        // Linear should produce different (smoother) result than none
        REQUIRE(!buffers_equal(output_none.data(), output_linear.data(), width * height));
    }
}

TEST_CASE("CoordinateLookupTable Script Execution", "[coordinate_grid]") {
    const int width = 32;
    const int height = 32;
    AudioData audio_data;
    memset(audio_data, 0, sizeof(AudioData));
    
    SECTION("Multi-statement script x=x; y=y-0.01") {
        CoordinateLookupTable table;
        
        // This should shift image up slightly
        table.generate(width, height, 8, 8,
                      "x=x; y=y-0.01", "x=x; y=y-0.01",  // Same script for both
                      true, false,
                      audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0);
        
        // Create horizontal stripe pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                input[y * width + x] = (y < height/2) ? 0xFFFF0000 : 0xFF0000FF;
            }
        }
        
        table.apply(input.data(), output.data(), width, height, false);
        
        // Check that pattern moved up (red stripe should be higher)
        int red_at_center = 0;
        for (int y = height/2 - 2; y < height/2 + 2; y++) {
            if ((output[y * width + width/2] & 0x00FF0000) == 0x00FF0000) {
                red_at_center++;
            }
        }
        
        // Some red should be visible near center after upward shift
        REQUIRE(red_at_center > 0);
    }
}

TEST_CASE("CoordinateLookupTable Diagnostic Tests", "[coordinate_grid]") {
    const int width = 8;  // Small size for detailed debugging
    const int height = 8;
    AudioData audio_data;
    memset(audio_data, 0, sizeof(AudioData));
    
    SECTION("Verify simple coordinate remapping") {
        CoordinateLookupTable table;
        
        // Generate simple identity transformation
        table.generate(width, height, width, height,  // Full resolution
                      "x", "y",  // Identity
                      true, false,
                      audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0);
        
        // Fill only corner pixels with unique colors
        std::fill(input.begin(), input.end(), 0xFF000000);  // Black
        input[0] = 0xFFFF0000;  // Top-left red
        input[width-1] = 0xFF00FF00;  // Top-right green  
        input[(height-1)*width] = 0xFF0000FF;  // Bottom-left blue
        input[(height-1)*width + (width-1)] = 0xFFFFFFFF;  // Bottom-right white
        
        table.apply(input.data(), output.data(), width, height, false);
        
        // For identity, corners should be preserved
        REQUIRE(output[0] == 0xFFFF0000);  // Top-left red
        REQUIRE(output[width-1] == 0xFF00FF00);  // Top-right green
        REQUIRE(output[(height-1)*width] == 0xFF0000FF);  // Bottom-left blue  
        REQUIRE(output[(height-1)*width + (width-1)] == 0xFFFFFFFF);  // Bottom-right white
    }
    
    SECTION("Test coordinate space mapping") {
        // Test that normalized [0,1] coordinates map correctly to pixel coordinates
        CoordinateLookupTable table;
        
        table.generate(width, height, width, height,  // Full resolution grid
                      "x*0.5+0.25", "y*0.5+0.25",  // Scale down and center
                      true, false,
                      audio_data, false, InterpolationMode::NONE);
        
        std::vector<uint32_t> input(width * height);
        std::vector<uint32_t> output(width * height, 0xFF000000);  // Black
        
        // Fill input with white
        std::fill(input.begin(), input.end(), 0xFFFFFFFF);
        
        table.apply(input.data(), output.data(), width, height, false);
        
        // Count white pixels in output - should be roughly 1/4 of total (scaled down by 0.5 in each dimension)
        int white_count = 0;
        for (int i = 0; i < width * height; i++) {
            if ((output[i] & 0x00FFFFFF) == 0x00FFFFFF) {
                white_count++;
            }
        }
        
        std::cout << "White pixels after 0.5 scale: " << white_count << " / " << (width * height) << std::endl;
        
        // Should have roughly 1/4 of the pixels
        REQUIRE(white_count > 0);
        REQUIRE(white_count < (width * height * 3 / 4));
    }
}