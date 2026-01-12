// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "shift_effect.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include <cmath>
#include <algorithm>

namespace avs {

ShiftEffect::ShiftEffect() {
    init_parameters_from_layout(effect_info.ui_layout);

    // Set default scripts from original AVS
    parameters().set_string("init_script", "d=0;");
    parameters().set_string("frame_script", "x=sin(d)*1.4; y=1.4*cos(d); d=d+0.01;");
    parameters().set_string("beat_script", "d=d+2.0");
}

void ShiftEffect::on_parameter_changed(const std::string& param_name) {
    if (param_name == "init_script") {
        inited_ = false;  // Re-run init on next render
    }
}

int ShiftEffect::render(AudioData visdata, int isBeat,
                        uint32_t* framebuffer, uint32_t* fbout,
                        int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    // Get scripts
    std::string init_script = parameters().get_string("init_script");
    std::string frame_script = parameters().get_string("frame_script");
    std::string beat_script = parameters().get_string("beat_script");

    // Set up variables
    engine_.set_variable("w", static_cast<double>(w));
    engine_.set_variable("h", static_cast<double>(h));
    engine_.set_variable("b", isBeat ? 1.0 : 0.0);

    // Run init code if first frame or size changed
    if (!inited_ || last_w_ != w || last_h_ != h) {
        engine_.set_variable("x", 0.0);
        engine_.set_variable("y", 0.0);
        engine_.set_variable("alpha", 0.5);
        if (!init_script.empty()) {
            engine_.evaluate(init_script);
        }
        inited_ = true;
        last_w_ = w;
        last_h_ = h;
    }

    // Run frame code
    if (!frame_script.empty()) {
        engine_.evaluate(frame_script);
    }

    // Run beat code if beat
    if (isBeat && !beat_script.empty()) {
        engine_.evaluate(beat_script);
    }

    // Get shift parameters
    double var_x = engine_.get_variable("x");
    double var_y = engine_.get_variable("y");
    double var_alpha = engine_.get_variable("alpha");

    bool blend = parameters().get_bool("blend");
    bool subpixel = parameters().get_bool("subpixel");

    int doblend = blend ? 1 : 0;
    int ialpha = 127;
    if (doblend) {
        ialpha = static_cast<int>(var_alpha * 255.0);
        if (ialpha <= 0) return 0;
        if (ialpha >= 255) doblend = 0;
    }

    uint32_t* inptr = framebuffer;
    uint32_t* blendptr = framebuffer;
    uint32_t* outptr = fbout;

    int xa = static_cast<int>(var_x);
    int ya = static_cast<int>(var_y);

    if (!subpixel) {
        // Non-subpixel (integer shift) mode
        int endy = h + ya;
        int endx = w + xa;
        if (endx > w) endx = w;
        if (endy > h) endy = h;
        if (ya < 0) inptr += -ya * w;
        if (ya > h) ya = h;
        if (xa > w) xa = w;

        int x, y;
        for (y = 0; y < ya; y++) {
            x = w;
            if (!doblend) {
                while (x--) *outptr++ = 0;
            } else {
                while (x--) *outptr++ = BLEND_ADJ(0, *blendptr++, ialpha);
            }
        }
        for (; y < endy; y++) {
            uint32_t* ip = inptr;
            if (xa < 0) inptr += -xa;
            if (!doblend) {
                for (x = 0; x < xa; x++) *outptr++ = 0;
                for (; x < endx; x++) *outptr++ = *inptr++;
                for (; x < w; x++) *outptr++ = 0;
            } else {
                for (x = 0; x < xa; x++) *outptr++ = BLEND_ADJ(0, *blendptr++, ialpha);
                for (; x < endx; x++) *outptr++ = BLEND_ADJ(*inptr++, *blendptr++, ialpha);
                for (; x < w; x++) *outptr++ = BLEND_ADJ(0, *blendptr++, ialpha);
            }
            inptr = ip + w;
        }
        for (; y < h; y++) {
            x = w;
            if (!doblend) {
                while (x--) *outptr++ = 0;
            } else {
                while (x--) *outptr++ = BLEND_ADJ(0, *blendptr++, ialpha);
            }
        }
    } else {
        // Subpixel (bilinear filtering) mode
        int xpart, ypart;

        {
            double vx = var_x;
            double vy = var_y;
            xpart = static_cast<int>((vx - static_cast<int>(vx)) * 255.0);
            if (xpart < 0) {
                xpart = -xpart;
            } else {
                xa++;
                xpart = 255 - xpart;
            }
            xpart = std::clamp(xpart, 0, 255);

            ypart = static_cast<int>((vy - static_cast<int>(vy)) * 255.0);
            if (ypart < 0) {
                ypart = -ypart;
            } else {
                ya++;
                ypart = 255 - ypart;
            }
            ypart = std::clamp(ypart, 0, 255);
        }

        xa = std::clamp(xa, 1 - w, w - 1);
        ya = std::clamp(ya, 1 - h, h - 1);
        if (ya < 0) inptr += -ya * w;

        int endy = h - 1 + ya;
        int endx = w - 1 + xa;
        endx = std::clamp(endx, 0, w - 1);
        endy = std::clamp(endy, 0, h - 1);

        int x, y;
        for (y = 0; y < ya; y++) {
            x = w;
            if (!doblend) {
                while (x--) *outptr++ = 0;
            } else {
                while (x--) *outptr++ = BLEND_ADJ(0, *blendptr++, ialpha);
            }
        }
        for (; y < endy; y++) {
            uint32_t* ip = inptr;
            if (xa < 0) inptr += -xa;
            if (!doblend) {
                for (x = 0; x < xa; x++) *outptr++ = 0;
                for (; x < endx; x++) {
                    *outptr++ = blend_bilinear_2x2(inptr++, w, xpart, ypart);
                }
                for (; x < w; x++) *outptr++ = 0;
            } else {
                for (x = 0; x < xa; x++) *outptr++ = BLEND_ADJ(0, *blendptr++, ialpha);
                for (; x < endx; x++) {
                    *outptr++ = BLEND_ADJ(blend_bilinear_2x2(inptr++, w, xpart, ypart), *blendptr++, ialpha);
                }
                for (; x < w; x++) *outptr++ = BLEND_ADJ(0, *blendptr++, ialpha);
            }
            inptr = ip + w;
        }
        for (; y < h; y++) {
            x = w;
            if (!doblend) {
                while (x--) *outptr++ = 0;
            } else {
                while (x--) *outptr++ = BLEND_ADJ(0, *blendptr++, ialpha);
            }
        }
    }

    return 1;  // Request buffer swap
}

void ShiftEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_shift.cpp:
    // First byte = version (1 = new format with length-prefixed strings)
    // If version == 1:
    //   3 length-prefixed strings (init, frame, beat)
    //   blend (int32)
    //   subpixel (int32)
    // Else (old format):
    //   768 bytes: 3x256 byte fixed strings

    if (data[0] == 1) {
        // New format with length-prefixed strings
        reader.skip(1);  // Skip version byte
        std::string init = reader.read_length_prefixed_string();
        std::string frame = reader.read_length_prefixed_string();
        std::string beat = reader.read_length_prefixed_string();

        parameters().set_string("init_script", init);
        parameters().set_string("frame_script", frame);
        parameters().set_string("beat_script", beat);
    } else {
        // Old format: 3x256 byte fixed strings
        if (data.size() >= 768) {
            char buf[257];
            buf[256] = 0;
            memcpy(buf, data.data(), 256);
            parameters().set_string("init_script", buf);
            memcpy(buf, data.data() + 256, 256);
            parameters().set_string("frame_script", buf);
            memcpy(buf, data.data() + 512, 256);
            parameters().set_string("beat_script", buf);
            reader.skip(768);
        }
    }

    if (reader.remaining() >= 4) {
        parameters().set_bool("blend", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("subpixel", reader.read_u32() != 0);
    }

    inited_ = false;  // Force re-init
}

// Static member definition
const PluginInfo ShiftEffect::effect_info {
    .name = "Dynamic Shift",
    .category = "Trans",
    .description = "Scripted pixel shift transformation",
    .author = "",
    .version = 1,
    .legacy_index = 42,  // R_Shift
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<ShiftEffect>();
    },
    .ui_layout = {
        {
            // Original UI from r_shift.cpp / res.rc IDD_CFG_SHIFT:
            // LTEXT labels + EDITTEXT for init/frame/beat scripts
            // IDC_CHECK1: blend, IDC_CHECK2: subpixel
            {
                .id = "init_label",
                .text = "init",
                .type = ControlType::LABEL,
                .x = 0, .y = 20, .w = 10, .h = 8,
                .default_val = 0
            },
            {
                .id = "init_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 29, .y = 0, .w = 204, .h = 52,
                .default_val = 0
            },
            {
                .id = "frame_label",
                .text = "frame",
                .type = ControlType::LABEL,
                .x = 0, .y = 83, .w = 18, .h = 8,
                .default_val = 0
            },
            {
                .id = "frame_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 29, .y = 52, .w = 204, .h = 70,
                .default_val = 0
            },
            {
                .id = "beat_label",
                .text = "beat",
                .type = ControlType::LABEL,
                .x = 0, .y = 150, .w = 15, .h = 8,
                .default_val = 0
            },
            {
                .id = "beat_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 29, .y = 123, .w = 204, .h = 63,
                .default_val = 0
            },
            {
                .id = "blend",
                .text = "Blend",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 194, .w = 34, .h = 10,
                .default_val = 0
            },
            {
                .id = "subpixel",
                .text = "Bilinear filtering",
                .type = ControlType::CHECKBOX,
                .x = 35, .y = 194, .w = 63, .h = 10,
                .default_val = 1
            }
        }
    },
    .help_text =
        "Dynamic Shift\n"
        "\n"
        "Variables:\n"
        "x, y = amount to shift (in pixels - set these)\n"
        "w, h = width, height (in pixels)\n"
        "b = isBeat (1 if beat, 0 otherwise)\n"
        "alpha = alpha value (0.0-1.0) for blend\n"
};

// Register effect at startup
static bool register_shift = []() {
    PluginManager::instance().register_effect(ShiftEffect::effect_info);
    return true;
}();

} // namespace avs
