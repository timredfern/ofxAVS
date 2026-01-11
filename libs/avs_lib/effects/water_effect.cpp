// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "water_effect.h"
#include "core/binary_reader.h"
#include "core/plugin_manager.h"
#include <algorithm>
#include <cstring>

namespace avs {

namespace {
// Channel extraction macros matching original
inline int get_r(uint32_t x) { return x & 0xFF; }
inline int get_g(uint32_t x) { return x & 0xFF00; }
inline int get_b(uint32_t x) { return x & 0xFF0000; }
inline uint32_t make_rgb(int r, int g, int b) {
    return (r & 0xFF) | (g & 0xFF00) | (b & 0xFF0000);
}
}  // namespace

WaterEffect::WaterEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int WaterEffect::render(AudioData visdata, int isBeat,
                         uint32_t* framebuffer, uint32_t* fbout,
                         int w, int h) {
    if (isBeat & 0x80000000) return 0;
    if (!is_enabled()) return 0;

    // Resize buffer if needed
    if (w != last_w_ || h != last_h_) {
        lastframe_.resize(w * h);
        std::fill(lastframe_.begin(), lastframe_.end(), 0);
        last_w_ = w;
        last_h_ = h;
    }

    uint32_t* f = framebuffer;
    uint32_t* of = fbout;
    uint32_t* lfo = lastframe_.data();

    // Top line
    {
        // Left edge
        {
            int r = get_r(f[1]);
            int g = get_g(f[1]);
            int b = get_b(f[1]);
            r += get_r(f[w]);
            g += get_g(f[w]);
            b += get_b(f[w]);
            f++;

            r -= get_r(lfo[0]);
            g -= get_g(lfo[0]);
            b -= get_b(lfo[0]);
            lfo++;

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255 * 256);
            b = std::clamp(b, 0, 255 * 65536);
            *of++ = make_rgb(r, g, b);
        }

        // Middle of top line
        for (int x = 0; x < w - 2; x++) {
            int r = get_r(f[1]);
            int g = get_g(f[1]);
            int b = get_b(f[1]);
            r += get_r(f[-1]);
            g += get_g(f[-1]);
            b += get_b(f[-1]);
            r += get_r(f[w]);
            g += get_g(f[w]);
            b += get_b(f[w]);
            f++;

            r /= 2;
            g /= 2;
            b /= 2;

            r -= get_r(lfo[0]);
            g -= get_g(lfo[0]);
            b -= get_b(lfo[0]);
            lfo++;

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255 * 256);
            b = std::clamp(b, 0, 255 * 65536);
            *of++ = make_rgb(r, g, b);
        }

        // Right edge
        {
            int r = get_r(f[-1]);
            int g = get_g(f[-1]);
            int b = get_b(f[-1]);
            r += get_r(f[w]);
            g += get_g(f[w]);
            b += get_b(f[w]);
            f++;

            r -= get_r(lfo[0]);
            g -= get_g(lfo[0]);
            b -= get_b(lfo[0]);
            lfo++;

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255 * 256);
            b = std::clamp(b, 0, 255 * 65536);
            *of++ = make_rgb(r, g, b);
        }
    }

    // Middle rows
    for (int y = 1; y < h - 1; y++) {
        // Left edge
        {
            int r = get_r(f[1]);
            int g = get_g(f[1]);
            int b = get_b(f[1]);
            r += get_r(f[w]);
            g += get_g(f[w]);
            b += get_b(f[w]);
            r += get_r(f[-w]);
            g += get_g(f[-w]);
            b += get_b(f[-w]);
            f++;

            r /= 2;
            g /= 2;
            b /= 2;

            r -= get_r(lfo[0]);
            g -= get_g(lfo[0]);
            b -= get_b(lfo[0]);
            lfo++;

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255 * 256);
            b = std::clamp(b, 0, 255 * 65536);
            *of++ = make_rgb(r, g, b);
        }

        // Middle pixels
        for (int x = 0; x < w - 2; x++) {
            int r = get_r(f[1]);
            int g = get_g(f[1]);
            int b = get_b(f[1]);
            r += get_r(f[-1]);
            g += get_g(f[-1]);
            b += get_b(f[-1]);
            r += get_r(f[w]);
            g += get_g(f[w]);
            b += get_b(f[w]);
            r += get_r(f[-w]);
            g += get_g(f[-w]);
            b += get_b(f[-w]);
            f++;

            r /= 2;
            g /= 2;
            b /= 2;

            r -= get_r(lfo[0]);
            g -= get_g(lfo[0]);
            b -= get_b(lfo[0]);
            lfo++;

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255 * 256);
            b = std::clamp(b, 0, 255 * 65536);
            *of++ = make_rgb(r, g, b);
        }

        // Right edge
        {
            int r = get_r(f[-1]);
            int g = get_g(f[-1]);
            int b = get_b(f[-1]);
            r += get_r(f[w]);
            g += get_g(f[w]);
            b += get_b(f[w]);
            r += get_r(f[-w]);
            g += get_g(f[-w]);
            b += get_b(f[-w]);
            f++;

            r /= 2;
            g /= 2;
            b /= 2;

            r -= get_r(lfo[0]);
            g -= get_g(lfo[0]);
            b -= get_b(lfo[0]);
            lfo++;

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255 * 256);
            b = std::clamp(b, 0, 255 * 65536);
            *of++ = make_rgb(r, g, b);
        }
    }

    // Bottom line
    {
        // Left edge
        {
            int r = get_r(f[1]);
            int g = get_g(f[1]);
            int b = get_b(f[1]);
            r += get_r(f[-w]);
            g += get_g(f[-w]);
            b += get_b(f[-w]);
            f++;

            r -= get_r(lfo[0]);
            g -= get_g(lfo[0]);
            b -= get_b(lfo[0]);
            lfo++;

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255 * 256);
            b = std::clamp(b, 0, 255 * 65536);
            *of++ = make_rgb(r, g, b);
        }

        // Middle of bottom line
        for (int x = 0; x < w - 2; x++) {
            int r = get_r(f[1]);
            int g = get_g(f[1]);
            int b = get_b(f[1]);
            r += get_r(f[-1]);
            g += get_g(f[-1]);
            b += get_b(f[-1]);
            r += get_r(f[-w]);
            g += get_g(f[-w]);
            b += get_b(f[-w]);
            f++;

            r /= 2;
            g /= 2;
            b /= 2;

            r -= get_r(lfo[0]);
            g -= get_g(lfo[0]);
            b -= get_b(lfo[0]);
            lfo++;

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255 * 256);
            b = std::clamp(b, 0, 255 * 65536);
            *of++ = make_rgb(r, g, b);
        }

        // Right edge
        {
            int r = get_r(f[-1]);
            int g = get_g(f[-1]);
            int b = get_b(f[-1]);
            r += get_r(f[-w]);
            g += get_g(f[-w]);
            b += get_b(f[-w]);
            f++;

            r -= get_r(lfo[0]);
            g -= get_g(lfo[0]);
            b -= get_b(lfo[0]);
            lfo++;

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255 * 256);
            b = std::clamp(b, 0, 255 * 65536);
            *of++ = make_rgb(r, g, b);
        }
    }

    // Save current frame for next iteration
    std::memcpy(lastframe_.data(), framebuffer, w * h * sizeof(uint32_t));

    return 1;  // Use fbout
}

void WaterEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    if (reader.remaining() >= 4) {
        int enabled = static_cast<int>(reader.read_u32());
        parameters().set_bool("enabled", enabled != 0);
    }
}

const PluginInfo WaterEffect::effect_info{
    .name = "Water",
    .category = "Trans",
    .description = "Water ripple effect",
    .author = "",
    .version = 1,
    .legacy_index = 20,
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<WaterEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable Water effect",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 79, .h = 10,
                .default_val = true
            }
        }
    }
};

// Register effect at startup
static bool register_water = []() {
    PluginManager::instance().register_effect(WaterEffect::effect_info);
    return true;
}();

}  // namespace avs
