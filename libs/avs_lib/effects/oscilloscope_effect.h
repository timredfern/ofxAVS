// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "../core/effect_base.h"

namespace avs {

class OscilloscopeEffect : public EffectBase {
public:
    OscilloscopeEffect();
    virtual ~OscilloscopeEffect() = default;
    
    // Core render function - simplified oscilloscope
    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;
    
    std::string get_name() const override { return "Oscilloscope"; }
    std::string get_description() const override { return "Audio waveform display"; }

private:
    int color_pos_ = 0;
    
    void setup_parameters();
    void draw_line(uint32_t* buffer, int w, int h, int x1, int y1, int x2, int y2, uint32_t color);
};

} // namespace avs