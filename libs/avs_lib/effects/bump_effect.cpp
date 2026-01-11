// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "bump_effect.h"
#include "core/plugin_manager.h"
#include "core/blend.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace avs {

BumpEffect::BumpEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
    this_depth_ = parameters().get_int("depth");
}

void BumpEffect::on_parameter_changed(const std::string& param_name) {
    if (param_name == "init_script") {
        inited_ = false;
    }
}

inline int BumpEffect::depthof(int c, bool invert) {
    // Depth is max of RGB channels
    int r = std::max(std::max((c & 0xFF), ((c >> 8) & 0xFF)), ((c >> 16) & 0xFF));
    return invert ? 255 - r : r;
}

inline int BumpEffect::setdepth(int l, int c) {
    int r = std::min((c & 0xFF) + l, 254);
    r |= std::min((c & 0xFF00) + (l << 8), 254 << 8);
    r |= std::min((c & 0xFF0000) + (l << 16), 254 << 16);
    return r;
}

inline int BumpEffect::setdepth0(int c) {
    int r = std::min(c & 0xFF, 254);
    r |= std::min(c & 0xFF00, 254 << 8);
    r |= std::min(c & 0xFF0000, 254 << 16);
    return r;
}

int BumpEffect::render(AudioData visdata, int isBeat,
                       uint32_t* framebuffer, uint32_t* fbout,
                       int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    // Get parameters
    int depth = parameters().get_int("depth");
    int depth2 = parameters().get_int("depth2");
    int durFrames = parameters().get_int("beat_duration");
    bool onbeat = parameters().get_bool("onbeat");
    int blend_mode = parameters().get_int("blend_mode");
    bool blend = (blend_mode == 1);     // Additive
    bool blendavg = (blend_mode == 2);  // 50/50
    bool showlight = parameters().get_bool("showlight");
    bool invert = parameters().get_bool("invert");
    int buffern = parameters().get_int("depth_buffer");

    // Set up script context
    script_engine_.set_audio_context(visdata, isBeat);

    // Set beat variables (original uses -1 for beat, 1 for no beat - weird but preserved)
    script_engine_.set_variable("isbeat", isBeat ? -1.0 : 1.0);
    script_engine_.set_variable("islbeat", nF_ ? -1.0 : 1.0);

    // Execute init script once
    if (!inited_) {
        script_engine_.evaluate(parameters().get_string("init_script"));
        inited_ = true;
    }

    // Execute frame script
    script_engine_.evaluate(parameters().get_string("frame_script"));

    // Execute beat script on beat
    if (isBeat) {
        script_engine_.evaluate(parameters().get_string("beat_script"));
    }

    // Handle on-beat depth transition
    if (onbeat && isBeat) {
        this_depth_ = depth2;
        nF_ = durFrames;
    } else if (!nF_) {
        this_depth_ = depth;
    }

    // Get depth buffer - currently only supports current framebuffer (buffern=0)
    // TODO: Support global buffers when implemented
    uint32_t* depthbuffer = framebuffer;
    bool curbuf = true;

    // Clear output buffer (original does this)
    std::memset(fbout, 0, w * h * sizeof(uint32_t));

    // Get light position from script variables (0-1 range, scaled to screen)
    double var_x = script_engine_.get_variable("x");
    double var_y = script_engine_.get_variable("y");

    int cx = static_cast<int>(var_x * w);
    int cy = static_cast<int>(var_y * h);
    cx = std::max(0, std::min(w - 1, cx));
    cy = std::max(0, std::min(h - 1, cy));

    // Show light position dot
    if (showlight) {
        fbout[cx + cy * w] = 0xFFFFFF;
    }

    // Apply bi (bump intensity) variable if set
    double bi = script_engine_.get_variable("bi");
    if (bi > 0) {
        bi = std::min(std::max(bi, 0.0), 1.0);
        this_depth_ = static_cast<int>(this_depth_ * bi);
    }

    int thisDepth_scaled = (this_depth_ << 8) / 100;

    // Process pixels (skip 1-pixel border)
    depthbuffer += w + 1;
    uint32_t* fb = framebuffer + w + 1;
    uint32_t* out = fbout + w + 1;

    int ly = 1 - cy;
    for (int y = 1; y < h - 1; y++) {
        int lx = 1 - cx;
        for (int x = 1; x < w - 1; x++) {
            // Get neighboring depth values
            int m1 = depthbuffer[-1];
            int p1 = depthbuffer[1];
            int mw = depthbuffer[-w];
            int pw = depthbuffer[w];

            // Skip if using current buffer and all neighbors are black
            if (!curbuf || (m1 || p1 || mw || pw)) {
                // Calculate bump lighting
                int coul1 = depthof(p1, invert) - depthof(m1, invert) - lx;
                int coul2 = depthof(pw, invert) - depthof(mw, invert) - ly;
                coul1 = 127 - std::abs(coul1);
                coul2 = 127 - std::abs(coul2);

                int pixel;
                if (coul1 <= 0 || coul2 <= 0) {
                    pixel = setdepth0(fb[0]);
                } else {
                    pixel = setdepth((coul1 * coul2 * thisDepth_scaled) >> (8 + 6), fb[0]);
                }

                // Apply blend mode
                if (blend) {
                    out[0] = BLEND(fb[0], pixel);
                } else if (blendavg) {
                    out[0] = BLEND_AVG(fb[0], pixel);
                } else {
                    out[0] = pixel;
                }
            }

            depthbuffer++;
            fb++;
            out++;
            lx++;
        }
        // Skip border pixels
        depthbuffer += 2;
        fb += 2;
        out += 2;
        ly++;
    }

    // Decay on-beat transition
    if (nF_) {
        nF_--;
        if (nF_) {
            int a = std::abs(depth - depth2) / durFrames;
            this_depth_ += a * (depth2 > depth ? -1 : 1);
        }
    }

    return 1;  // Output in fbout
}

// Static member definition - UI layout from res.rc IDD_CFG_BUMP
const PluginInfo BumpEffect::effect_info {
    .name = "Bump",
    .category = "Trans",
    .description = "Bump map lighting effect with scripted light position",
    .author = "",
    .version = 1,
    .legacy_index = 29,  // R_Bump
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<BumpEffect>();
    },
    .ui_layout = {
        {
            // Enable checkbox
            {
                .id = "enabled",
                .text = "Enable Bump",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 56, .h = 10,
                .default_val = 1
            },
            // Invert depth checkbox
            {
                .id = "invert",
                .text = "Invert depth",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 10, .w = 54, .h = 10,
                .default_val = 0
            },
            // Depth slider (Flat/Bumpy)
            {
                .id = "depth",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 58, .y = 11, .w = 79, .h = 11,
                .range = {1, 100, 1},
                .default_val = 30
            },
            // Show Dot checkbox
            {
                .id = "showlight",
                .text = "Show Dot",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 20, .w = 47, .h = 10,
                .default_val = 0
            },
            // Depth slider labels
            {
                .id = "flat_label",
                .text = "Flat",
                .type = ControlType::LABEL,
                .x = 61, .y = 22, .w = 12, .h = 8
            },
            {
                .id = "bumpy_label",
                .text = "Bumpy",
                .type = ControlType::LABEL,
                .x = 115, .y = 22, .w = 22, .h = 8
            },
            // Light position label
            {
                .id = "lightpos_label",
                .text = "Light position:",
                .type = ControlType::LABEL,
                .x = 0, .y = 38, .w = 44, .h = 8
            },
            // Init script
            {
                .id = "init_label",
                .text = "init",
                .type = ControlType::LABEL,
                .x = 0, .y = 63, .w = 10, .h = 8
            },
            {
                .id = "init_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 24, .y = 52, .w = 209, .h = 33,
                .default_val = "t=0;"
            },
            // Frame script
            {
                .id = "frame_label",
                .text = "frame",
                .type = ControlType::LABEL,
                .x = 0, .y = 98, .w = 18, .h = 8
            },
            {
                .id = "frame_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 24, .y = 87, .w = 209, .h = 48,
                .default_val = "x=0.5+cos(t)*0.3;\r\ny=0.5+sin(t)*0.3;\r\nt=t+0.1;"
            },
            // Beat script
            {
                .id = "beat_label",
                .text = "beat",
                .type = ControlType::LABEL,
                .x = 0, .y = 147, .w = 15, .h = 8
            },
            {
                .id = "beat_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 24, .y = 136, .w = 209, .h = 40,
                .default_val = ""
            },
            // Depth buffer dropdown
            {
                .id = "buffer_label",
                .text = "Depth buffer:",
                .type = ControlType::LABEL,
                .x = 0, .y = 181, .w = 48, .h = 8
            },
            {
                .id = "depth_buffer",
                .text = "",
                .type = ControlType::DROPDOWN,
                .x = 52, .y = 179, .w = 85, .h = 56,
                .default_val = 0,
                .options = {
                    "Current", "Buffer 1", "Buffer 2", "Buffer 3", "Buffer 4",
                    "Buffer 5", "Buffer 6", "Buffer 7", "Buffer 8"
                }
            },
            // Blend mode radio buttons
            {
                .id = "blend_mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Replace", 148, 0, 43, 10},
                    {"Additive blend", 148, 9, 61, 10},
                    {"Blend 50/50", 148, 19, 55, 10}
                },
                .default_val = 0  // Replace
            },
            // OnBeat section
            {
                .id = "onbeat",
                .text = "OnBeat",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 198, .w = 40, .h = 10,
                .default_val = 0
            },
            // Beat duration slider (Shorter/Longer)
            {
                .id = "beat_duration",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 44, .y = 196, .w = 67, .h = 11,
                .range = {1, 100, 1},
                .default_val = 15
            },
            // OnBeat depth slider
            {
                .id = "depth2",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 114, .y = 196, .w = 67, .h = 11,
                .range = {1, 100, 1},
                .default_val = 100
            }
        }
    }
};

// Register effect at startup
static bool register_bump = []() {
    PluginManager::instance().register_effect(BumpEffect::effect_info);
    return true;
}();

} // namespace avs
