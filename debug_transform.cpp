#include <iostream>
#include <vector>
#include "libs/avs_lib/core/coordinate_lookup_table.h"

int main() {
    using namespace avs;
    
    // Create a small test case
    const int width = 4;
    const int height = 4;
    const int grid_width = 2;
    const int grid_height = 2;
    
    AudioData dummy_audio = {};
    CoordinateLookupTable table;
    
    // Simple identity transform
    table.generate(width, height, grid_width, grid_height, "x", "y", true, false, dummy_audio, false, InterpolationMode::NONE);
    
    std::cout << "Grid lookup table values:" << std::endl;
    for (int gy = 0; gy < grid_height; gy++) {
        for (int gx = 0; gx < grid_width; gx++) {
            uint32_t lookup = table.get_lookup(gx, gy);
            int src_x = lookup % width;
            int src_y = lookup / width;
            std::cout << "Grid[" << gx << "," << gy << "] -> Pixel[" << src_x << "," << src_y << "] (offset=" << lookup << ")" << std::endl;
        }
    }
    
    // Test apply method with simple input
    std::vector<uint32_t> input(width * height, 0xFF00FF00); // Green pixels
    std::vector<uint32_t> output(width * height, 0xFF000000); // Black pixels
    
    input[0] = 0xFFFF0000; // Red pixel at top-left
    input[width-1] = 0xFF0000FF; // Blue pixel at top-right
    
    table.apply(input.data(), output.data(), width, height, false);
    
    std::cout << "\nInput:" << std::endl;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t pixel = input[y * width + x];
            std::cout << std::hex << pixel << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\nOutput:" << std::endl;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t pixel = output[y * width + x];
            std::cout << std::hex << pixel << " ";
        }
        std::cout << std::endl;
    }
    
    return 0;
}