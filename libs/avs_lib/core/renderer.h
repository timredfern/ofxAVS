// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "effect_base.h"
#include "effect_container.h"
#include "../effects/effect_list_root.h"
#include <memory>
#include <vector>
#include <cstdint>

namespace avs {

// Templated pixel format for different bit depths
template<typename T>
struct RGBAPixel {
    T r, g, b, a;
    
    // Constructor - default to black (alpha handled at output stage)
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
    
    // Root effect list access
    EffectListRoot* root() { return root_.get(); }
    const EffectListRoot* root() const { return root_.get(); }

    // Effect chain management - delegates to root
    void add_effect(std::unique_ptr<EffectBase> effect) {
        root_->add_child(std::move(effect));
    }
    void insert_effect(size_t index, std::unique_ptr<EffectBase> effect) {
        root_->insert_child(index, std::move(effect));
    }
    void remove_effect(size_t index) {
        root_->remove_child(index);
    }
    void clear_effects() {
        while (root_->child_count() > 0) {
            root_->remove_child(0);
        }
    }
    size_t effect_count() const { return root_->child_count(); }

    // Access effects for UI/parameter modification - delegates to root
    EffectBase* get_effect(size_t index) {
        return root_->get_child(index);
    }
    const EffectBase* get_effect(size_t index) const {
        return root_->get_child(index);
    }

    // Swap two effects in the chain - delegates to root
    void swap_effects(size_t index_a, size_t index_b) {
        root_->swap_children(index_a, index_b);
    }
    
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

    // Root effect list (the "Main" container)
    std::unique_ptr<EffectListRoot> root_;

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
    root_ = std::make_unique<EffectListRoot>();
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
void Renderer<PixelType>::resize(int width, int height) {
    width_ = width;
    height_ = height;
    allocate_buffers();
}

template<typename PixelType>
void Renderer<PixelType>::render(AudioData visdata, bool is_beat, PixelType* output_buffer) {
    if (!root_ || root_->child_count() == 0) {
        // No effects - clear output buffer
        std::fill_n(output_buffer, width_ * height_, PixelType{});
        return;
    }

    // Don't auto-clear buffer - preserve previous frame for feedback effects
    // The Clear effect or EffectList clear_each_frame will handle clearing when needed

    // Render the root effect list - it handles buffer swapping for its children
    // Result is always in framebuffer (buffer_a) per EffectList convention
    root_->render(visdata, is_beat ? 1 : 0,
                  reinterpret_cast<uint32_t*>(get_buffer_a()),
                  reinterpret_cast<uint32_t*>(get_buffer_b()),
                  width_, height_);

    // Copy final result to output
    std::memcpy(output_buffer, get_buffer_a(), width_ * height_ * sizeof(PixelType));
}

template<typename PixelType>
void Renderer<PixelType>::render(AudioData visdata, bool is_beat, uint32_t* output_buffer) {
    // Legacy compatibility - convert to PixelType temporarily
    std::vector<PixelType> temp_buffer(width_ * height_);
    render(visdata, is_beat, temp_buffer.data());

    // Convert back to uint32_t, forcing alpha to opaque
    // Original AVS ignored alpha for display (used BitBlt SRCCOPY)
    // We need opaque pixels for OpenFrameworks alpha blending
    for (size_t i = 0; i < temp_buffer.size(); ++i) {
        output_buffer[i] = static_cast<uint32_t>(temp_buffer[i]) | 0xFF000000;
    }
}

// Default to 8-bit RGBA for now
using DefaultRenderer = Renderer<RGBA8>;

} // namespace avs