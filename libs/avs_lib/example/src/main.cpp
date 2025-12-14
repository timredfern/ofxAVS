#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>

#include "core/renderer.h"
#include "core/plugin_manager.h"
#include "core/builtin_effects.h"

// Simple PPM image writer for testing
void save_ppm(const std::string& filename, uint32_t* pixels, int width, int height) {
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) return;
    
    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    
    for (int i = 0; i < width * height; i++) {
        uint32_t pixel = pixels[i];
        unsigned char rgb[3] = {
            (unsigned char)((pixel >> 16) & 0xFF), // R
            (unsigned char)((pixel >> 8) & 0xFF),  // G
            (unsigned char)(pixel & 0xFF)          // B
        };
        fwrite(rgb, 1, 3, fp);
    }
    
    fclose(fp);
}

// Generate synthetic audio data for testing
void generate_test_audio(avs::AudioData& data, double time) {
    // Generate sine wave for testing
    for (int channel = 0; channel < 2; channel++) {
        // Waveform data (time domain)
        for (int i = 0; i < 576; i++) {
            double t = time + (double)i / 576.0;
            double amplitude = 128.0 * sin(2.0 * M_PI * 440.0 * t); // 440Hz sine wave
            data[channel][1][i] = (char)(amplitude + 128); // Convert to unsigned char range
        }
        
        // Spectrum data (simplified - just put some energy in low frequencies)
        for (int i = 0; i < 576; i++) {
            if (i < 32) {
                // Low frequencies - simulate bass
                data[channel][0][i] = (char)(64 * sin(time * 2.0) + 64);
            } else if (i < 128) {
                // Mid frequencies
                data[channel][0][i] = (char)(32 * sin(time * 4.0) + 32);
            } else {
                // High frequencies - mostly quiet
                data[channel][0][i] = 16;
            }
        }
    }
}

int main() {
    std::cout << "AVS Port - Standalone Example\n";
    std::cout << "============================\n\n";
    
    // Register built-in effects
    avs::register_builtin_effects();
    
    // Initialize plugin manager and check available effects
    auto& plugin_mgr = avs::PluginManager::instance();
    auto effects = plugin_mgr.available_effects();
    
    std::cout << "Available effects:\n";
    for (const auto& effect : effects) {
        auto info = plugin_mgr.get_effect_info(effect);
        std::cout << "  - " << effect << ": " << info.name << "\n";
    }
    std::cout << "\n";
    
    // Create renderer
    const int width = 400;
    const int height = 300;
    avs::Renderer renderer(width, height);
    
    // Add effects
    auto clear_effect = plugin_mgr.create_effect("clear");
    if (clear_effect) {
        // Set clear color to black
        clear_effect->parameters().set_color("color", 0x000000);
        renderer.add_effect(std::move(clear_effect));
        std::cout << "Added Clear effect\n";
    }
    
    auto osc_effect = plugin_mgr.create_effect("oscilloscope");
    if (osc_effect) {
        // Set oscilloscope to draw lines instead of dots
        osc_effect->parameters().set_bool("solid", true);
        osc_effect->parameters().set_color("color", 0x00FF00); // Green
        renderer.add_effect(std::move(osc_effect));
        std::cout << "Added Oscilloscope effect\n";
    }
    
    auto blur_effect = plugin_mgr.create_effect("blur");
    if (blur_effect) {
        // Set blur parameters
        blur_effect->parameters().set_float("strength", 0.3);
        blur_effect->parameters().set_int("radius", 2);
        renderer.add_effect(std::move(blur_effect));
        std::cout << "Added Blur effect\n";
    }
    
    // Render a few frames
    std::vector<uint32_t> framebuffer(width * height);
    avs::AudioData audio_data;
    
    std::cout << "\nRendering frames...\n";
    for (int frame = 0; frame < 5; frame++) {
        double time = frame * 0.1; // 10 fps simulation
        
        // Generate test audio data
        generate_test_audio(audio_data, time);
        
        // Simulate occasional beats
        bool is_beat = (frame % 3 == 0);
        
        // Render frame
        renderer.render(audio_data, is_beat, framebuffer.data());
        
        // Save frame as PPM image
        std::string filename = "frame_" + std::to_string(frame) + ".ppm";
        save_ppm(filename, framebuffer.data(), width, height);
        
        std::cout << "  Rendered " << filename << (is_beat ? " (beat)" : "") << "\n";
    }
    
    std::cout << "\nDone! Check the generated .ppm files.\n";
    std::cout << "You can convert them to PNG with: convert frame_0.ppm frame_0.png\n";
    
    return 0;
}