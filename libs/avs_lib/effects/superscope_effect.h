// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "../core/effect_base.h"
#include "../core/script/script_engine.h"
#include <array>

namespace avs {
struct PluginInfo;
}

namespace avs {

class SuperScopeEffect : public EffectBase {
public:
    SuperScopeEffect();
    virtual ~SuperScopeEffect() = default;

    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    // Binary config loading from legacy AVS presets
    void load_parameters(const std::vector<uint8_t>& data) override;

    static const PluginInfo effect_info;

private:
    ScriptEngine engine_;
    bool inited_ = false;
    int color_pos_ = 0;

    // Cached script strings for change detection
    std::string last_init_script_;
    std::string last_point_script_;
    std::string last_frame_script_;
    std::string last_beat_script_;

    void draw_line(uint32_t* buffer, int w, int h,
                   int x1, int y1, int x2, int y2, uint32_t color);

    // Helper to convert 0-1 color to 0-255
    static int make_color_component(double val) {
        if (val <= 0.0) return 0;
        if (val >= 1.0) return 255;
        return static_cast<int>(val * 255.0);
    }
};

} // namespace avs
