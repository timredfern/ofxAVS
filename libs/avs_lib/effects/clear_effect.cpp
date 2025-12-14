#include "clear_effect.h"
#include "../core/plugin_manager.h"
#include <memory>

namespace avs {

ClearEffect::ClearEffect() {
    setup_parameters();
}

void ClearEffect::setup_parameters() {
    // Set up parameters matching original clear effect
    auto& params = parameters();
    
    params.add_parameter(std::make_shared<Parameter>("enabled", ParameterType::BOOL, true));
    params.add_parameter(std::make_shared<Parameter>("color", ParameterType::COLOR, uint32_t(0xFF000000))); // Black with full alpha
    params.add_parameter(std::make_shared<Parameter>("only_first", ParameterType::BOOL, false));
    params.add_parameter(std::make_shared<Parameter>("blend_mode", ParameterType::INT, 0, 0, 3)); // 0=replace, 1=add, 2=line, 3=avg
}

// Blend macros ported from original (simplified for now)
static uint32_t blend_replace(uint32_t dest, uint32_t src) { return src; }
static uint32_t blend_add(uint32_t dest, uint32_t src) {
    int r = ((dest >> 16) & 0xFF) + ((src >> 16) & 0xFF);
    int g = ((dest >> 8) & 0xFF) + ((src >> 8) & 0xFF);
    int b = (dest & 0xFF) + (src & 0xFF);
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (r << 16) | (g << 8) | b;
}
static uint32_t blend_avg(uint32_t dest, uint32_t src) {
    int r = (((dest >> 16) & 0xFF) + ((src >> 16) & 0xFF)) / 2;
    int g = (((dest >> 8) & 0xFF) + ((src >> 8) & 0xFF)) / 2;
    int b = ((dest & 0xFF) + (src & 0xFF)) / 2;
    return (r << 16) | (g << 8) | b;
}

int ClearEffect::render(AudioData visdata, int isBeat,
                       uint32_t* framebuffer, uint32_t* fbout,
                       int w, int h) {
    // Port of original render logic with minimal changes
    if (!is_enabled()) return 0;
    
    bool only_first = parameters().get_bool("only_first");
    if (only_first && frame_counter_ > 0) return 0;
    
    if (isBeat & 0x80000000) return 0; // Original beat check
    
    frame_counter_++;
    
    uint32_t color = parameters().get_color("color");
    int blend_mode = parameters().get_int("blend_mode");
    
    int pixel_count = w * h;
    uint32_t* p = framebuffer;
    
    // Apply clearing operation based on blend mode
    switch (blend_mode) {
        case 1: // Additive
            for (int i = 0; i < pixel_count; i++) {
                p[i] = blend_add(p[i], color);
            }
            break;
        case 2: // Line blend (simplified for now)
            for (int i = 0; i < pixel_count; i++) {
                p[i] = blend_add(p[i], color);
            }
            break;
        case 3: // Average
            for (int i = 0; i < pixel_count; i++) {
                p[i] = blend_avg(p[i], color);
            }
            break;
        default: // Replace
            for (int i = 0; i < pixel_count; i++) {
                p[i] = color;
            }
            break;
    }
    
    return 0; // Use input buffer (modified in place)
}

// Register the effect
REGISTER_AVS_EFFECT("clear", ClearEffect);

} // namespace avs