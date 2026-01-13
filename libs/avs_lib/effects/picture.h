// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Port of r_picture.cpp from original AVS

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"
#include <vector>
#include <string>

namespace avs {

/**
 * Picture Effect - Display image files as background/overlay
 *
 * Loads image files (JPEG, PNG, BMP, GIF, TGA, etc.) and displays
 * them with various blend modes. Supports aspect ratio preservation
 * and beat-reactive blending.
 */
class PictureEffect : public EffectBase {
public:
    PictureEffect();
    ~PictureEffect();

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    void load_parameters(const std::vector<uint8_t>& data) override;
    void on_parameter_changed(const std::string& name) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    static const PluginInfo effect_info;

private:
    void load_image(const std::string& filename);
    void free_image();
    void update_scaled_image(int w, int h);

    // Original image data
    std::vector<uint32_t> image_data_;
    int image_width_ = 0;
    int image_height_ = 0;

    // Scaled image cache
    std::vector<uint32_t> scaled_data_;
    int last_width_ = 0;
    int last_height_ = 0;

    // Beat persistence counter
    int persist_count_ = 0;
};

} // namespace avs
