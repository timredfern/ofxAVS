#include "renderer.h"
#include <algorithm>
#include <cstring>

namespace avs {

Renderer::Renderer(int width, int height) 
    : width_(width), height_(height) {
    allocate_buffers();
}

Renderer::~Renderer() = default;

void Renderer::allocate_buffers() {
    size_t buffer_size = width_ * height_;
    buffer_a_.resize(buffer_size);
    buffer_b_.resize(buffer_size);
}

void Renderer::add_effect(std::unique_ptr<EffectBase> effect) {
    if (effect) {
        effects_.push_back(std::move(effect));
    }
}

void Renderer::remove_effect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void Renderer::clear_effects() {
    effects_.clear();
}

void Renderer::resize(int width, int height) {
    width_ = width;
    height_ = height;
    allocate_buffers();
}

void Renderer::render(AudioData visdata, bool is_beat, uint32_t* output_buffer) {
    if (effects_.empty()) {
        // No effects - clear output buffer
        std::fill_n(output_buffer, width_ * height_, 0);
        return;
    }
    
    // Start with clear buffer
    std::fill(buffer_a_.begin(), buffer_a_.end(), 0);
    
    // Chain effects using double buffering
    uint32_t* current_input = get_buffer_a();
    uint32_t* current_output = get_buffer_b();
    bool use_buffer_a = true;
    
    for (auto& effect : effects_) {
        if (!effect->is_enabled()) continue;
        
        int result = effect->render(visdata, is_beat ? 1 : 0, 
                                   current_input, current_output, 
                                   width_, height_);
        
        // Handle result according to AVS convention:
        // 0 = use input buffer, 1 = use output buffer
        if (result == 1) {
            // Effect wrote to output buffer, swap for next effect
            std::swap(current_input, current_output);
            use_buffer_a = !use_buffer_a;
        }
        // If result == 0, next effect reads from same input buffer
    }
    
    // Copy final result to output
    std::memcpy(output_buffer, current_input, width_ * height_ * sizeof(uint32_t));
}

} // namespace avs