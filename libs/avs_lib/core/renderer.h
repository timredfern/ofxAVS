#pragma once

#include "effect_base.h"
#include <memory>
#include <vector>

namespace avs {

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    
    // Effect chain management
    void add_effect(std::unique_ptr<EffectBase> effect);
    void remove_effect(size_t index);
    void clear_effects();
    size_t effect_count() const { return effects_.size(); }
    
    // Main render call
    void render(AudioData visdata, bool is_beat, uint32_t* output_buffer);
    
    // Dimensions
    int width() const { return width_; }
    int height() const { return height_; }
    void resize(int width, int height);

private:
    int width_;
    int height_;
    
    // Double buffering for effect chain
    std::vector<uint32_t> buffer_a_;
    std::vector<uint32_t> buffer_b_;
    
    // Effect chain
    std::vector<std::unique_ptr<EffectBase>> effects_;
    
    // Helper to get buffer pointers
    uint32_t* get_buffer_a() { return buffer_a_.data(); }
    uint32_t* get_buffer_b() { return buffer_b_.data(); }
    
    void allocate_buffers();
};

} // namespace avs