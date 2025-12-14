#pragma once

#include "effect_base.h"
#include <memory>
#include <vector>
#include <cstdint>

namespace avs {

// Templated pixel format for different bit depths
template<typename T>
struct RGBAPixel {
    T r, g, b, a;
    
    // Constructor
    RGBAPixel() : r(0), g(0), b(0), a(0) {}
    RGBAPixel(T red, T green, T blue, T alpha = 0xFF) : r(red), g(green), b(blue), a(alpha) {}
    
    // Conversion to/from uint32_t (for compatibility)
    explicit operator uint32_t() const {
        return (static_cast<uint32_t>(r) << 16) | 
               (static_cast<uint32_t>(g) << 8) | 
               (static_cast<uint32_t>(b)) | 
               (static_cast<uint32_t>(a) << 24);
    }
    
    RGBAPixel(uint32_t rgba) {
        r = static_cast<T>((rgba >> 16) & 0xFF);
        g = static_cast<T>((rgba >> 8) & 0xFF);
        b = static_cast<T>(rgba & 0xFF);
        a = static_cast<T>((rgba >> 24) & 0xFF);
    }
};

// Standard formats
using RGBA8 = RGBAPixel<uint8_t>;
using RGBA16 = RGBAPixel<uint16_t>;
using RGBAFloat = RGBAPixel<float>;

}

namespace avs {

template<typename PixelType = RGBA8>
class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    
    // Effect chain management
    void add_effect(std::unique_ptr<EffectBase> effect);
    void remove_effect(size_t index);
    void clear_effects();
    size_t effect_count() const { return effects_.size(); }
    
    // Main render call - now templated for pixel type
    void render(AudioData visdata, bool is_beat, PixelType* output_buffer);
    
    // Legacy uint32_t compatibility
    void render(AudioData visdata, bool is_beat, uint32_t* output_buffer);
    
    // Dimensions
    int width() const { return width_; }
    int height() const { return height_; }
    void resize(int width, int height);

private:
    int width_;
    int height_;
    
    // Double buffering for effect chain - now templated
    std::vector<PixelType> buffer_a_;
    std::vector<PixelType> buffer_b_;
    
    // Effect chain
    std::vector<std::unique_ptr<EffectBase>> effects_;
    
    // Helper to get buffer pointers
    PixelType* get_buffer_a() { return buffer_a_.data(); }
    PixelType* get_buffer_b() { return buffer_b_.data(); }
    
    void allocate_buffers();
};

// Template implementation
template<typename PixelType>
Renderer<PixelType>::Renderer(int width, int height) 
    : width_(width), height_(height) {
    allocate_buffers();
}

template<typename PixelType>
Renderer<PixelType>::~Renderer() = default;

template<typename PixelType>
void Renderer<PixelType>::allocate_buffers() {
    size_t buffer_size = width_ * height_;
    buffer_a_.resize(buffer_size);
    buffer_b_.resize(buffer_size);
}

template<typename PixelType>
void Renderer<PixelType>::add_effect(std::unique_ptr<EffectBase> effect) {
    if (effect) {
        effects_.push_back(std::move(effect));
    }
}

template<typename PixelType>
void Renderer<PixelType>::remove_effect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

template<typename PixelType>
void Renderer<PixelType>::clear_effects() {
    effects_.clear();
}

template<typename PixelType>
void Renderer<PixelType>::resize(int width, int height) {
    width_ = width;
    height_ = height;
    allocate_buffers();
}

template<typename PixelType>
void Renderer<PixelType>::render(AudioData visdata, bool is_beat, PixelType* output_buffer) {
    if (effects_.empty()) {
        // No effects - clear output buffer
        std::fill_n(output_buffer, width_ * height_, PixelType{});
        return;
    }
    
    // Don't auto-clear buffer - preserve previous frame for feedback effects
    // The Clear effect will handle clearing when needed
    
    // Chain effects using double buffering
    PixelType* current_input = get_buffer_a();
    PixelType* current_output = get_buffer_b();
    bool use_buffer_a = true;
    
    for (auto& effect : effects_) {
        if (!effect->is_enabled()) continue;
        
        int result = effect->render(visdata, is_beat ? 1 : 0, 
                                   reinterpret_cast<uint32_t*>(current_input), 
                                   reinterpret_cast<uint32_t*>(current_output), 
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
    std::memcpy(output_buffer, current_input, width_ * height_ * sizeof(PixelType));
    
    // Preserve final result in buffer_a for next frame (feedback)
    if (current_input != get_buffer_a()) {
        std::memcpy(get_buffer_a(), current_input, width_ * height_ * sizeof(PixelType));
    }
}

template<typename PixelType>
void Renderer<PixelType>::render(AudioData visdata, bool is_beat, uint32_t* output_buffer) {
    // Legacy compatibility - convert to PixelType temporarily
    std::vector<PixelType> temp_buffer(width_ * height_);
    render(visdata, is_beat, temp_buffer.data());
    
    // Convert back to uint32_t
    for (size_t i = 0; i < temp_buffer.size(); ++i) {
        output_buffer[i] = static_cast<uint32_t>(temp_buffer[i]);
    }
}

// Default to 8-bit RGBA for now
using DefaultRenderer = Renderer<RGBA8>;

} // namespace avs