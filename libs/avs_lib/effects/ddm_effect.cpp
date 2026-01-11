// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ddm_effect.h"
#include "core/binary_reader.h"
#include "core/plugin_manager.h"
#include "core/blend.h"
#include <algorithm>
#include <cstring>

namespace avs {

// Integer square root lookup table from original AVS (r_ddm.cpp)
// Based on 80386 Assembly implementation by Arne Steinarson
const unsigned char DDMEffect::sq_table_[256] = {
    0, 16, 22, 27, 32, 35, 39, 42, 45, 48, 50, 53, 55, 57, 59, 61, 64, 65,
    67, 69, 71, 73, 75, 76, 78, 80, 81, 83, 84, 86, 87, 89, 90, 91, 93, 94,
    96, 97, 98, 99, 101, 102, 103, 104, 106, 107, 108, 109, 110, 112, 113,
    114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 128,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141,
    142, 143, 144, 144, 145, 146, 147, 148, 149, 150, 150, 151, 152, 153,
    154, 155, 155, 156, 157, 158, 159, 160, 160, 161, 162, 163, 163, 164,
    165, 166, 167, 167, 168, 169, 170, 170, 171, 172, 173, 173, 174, 175,
    176, 176, 177, 178, 178, 179, 180, 181, 181, 182, 183, 183, 184, 185,
    185, 186, 187, 187, 188, 189, 189, 190, 191, 192, 192, 193, 193, 194,
    195, 195, 196, 197, 197, 198, 199, 199, 200, 201, 201, 202, 203, 203,
    204, 204, 205, 206, 206, 207, 208, 208, 209, 209, 210, 211, 211, 212,
    212, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 219, 220,
    221, 221, 222, 222, 223, 224, 224, 225, 225, 226, 226, 227, 227, 228,
    229, 229, 230, 230, 231, 231, 232, 232, 233, 234, 234, 235, 235, 236,
    236, 237, 237, 238, 238, 239, 240, 240, 241, 241, 242, 242, 243, 243,
    244, 244, 245, 245, 246, 246, 247, 247, 248, 248, 249, 249, 250, 250,
    251, 251, 252, 252, 253, 253, 254, 254, 255
};

// Integer square root from original AVS - uses factoring with lookup table
unsigned long DDMEffect::isqrt(unsigned long n) {
    if (n >= 0x10000)
        if (n >= 0x1000000)
            if (n >= 0x10000000)
                if (n >= 0x40000000) return (sq_table_[n >> 24] << 8);
                else                 return (sq_table_[n >> 22] << 7);
            else
                if (n >= 0x4000000)  return (sq_table_[n >> 20] << 6);
                else                 return (sq_table_[n >> 18] << 5);
        else
            if (n >= 0x100000)
                if (n >= 0x400000)   return (sq_table_[n >> 16] << 4);
                else                 return (sq_table_[n >> 14] << 3);
            else
                if (n >= 0x40000)    return (sq_table_[n >> 12] << 2);
                else                 return (sq_table_[n >> 10] << 1);
    else
        if (n >= 0x100)
            if (n >= 0x1000)
                if (n >= 0x4000)     return (sq_table_[n >> 8]);
                else                 return (sq_table_[n >> 6] >> 1);
            else
                if (n >= 0x400)      return (sq_table_[n >> 4] >> 2);
                else                 return (sq_table_[n >> 2] >> 3);
        else
            if (n >= 0x10)
                if (n >= 0x40)       return (sq_table_[n] >> 4);
                else                 return (sq_table_[n << 2] >> 5);
            else
                if (n >= 0x4)        return (sq_table_[n << 4] >> 6);
                else                 return (sq_table_[n << 6] >> 7);
}

DDMEffect::DDMEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

void DDMEffect::on_parameter_changed(const std::string& param_name) {
    if (param_name == "init_script") {
        inited_ = false;
    }
}

void DDMEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_ddm.cpp:
    // If first byte == 1: new format with length-prefixed strings
    // Else: old format with 2 x 512 byte blocks (4 x 256 byte strings)
    // Order: effect_exp[0]=pixel, [1]=frame, [2]=beat, [3]=init
    std::string pixel_script, frame_script, beat_script, init_script;

    if (reader.read_u8() == 1) {
        // New format: length-prefixed strings
        pixel_script = reader.read_length_prefixed_string();
        frame_script = reader.read_length_prefixed_string();
        beat_script = reader.read_length_prefixed_string();
        init_script = reader.read_length_prefixed_string();
    } else {
        // Old format: 2 x 512 byte blocks
        // First block: effect_exp[0] (256 bytes) + effect_exp[1] (256 bytes)
        // Second block: effect_exp[2] (256 bytes) + effect_exp[3] (256 bytes)
        BinaryReader reader2(data);  // Fresh reader from start
        if (reader2.remaining() >= 512) {
            pixel_script = reader2.read_string_fixed(256);
            frame_script = reader2.read_string_fixed(256);
        }
        if (reader2.remaining() >= 512) {
            beat_script = reader2.read_string_fixed(256);
            init_script = reader2.read_string_fixed(256);
        }
    }

    parameters().set_string("pixel_script", pixel_script);
    parameters().set_string("frame_script", frame_script);
    parameters().set_string("beat_script", beat_script);
    parameters().set_string("init_script", init_script);

    // After strings: blend (int32), subpixel (int32)
    if (reader.remaining() >= 4) {
        int blend = reader.read_u32();
        parameters().set_bool("blend", blend != 0);
    }
    if (reader.remaining() >= 4) {
        int subpixel = reader.read_u32();
        parameters().set_bool("bilinear", subpixel != 0);
    }

    // Trigger init script re-execution
    inited_ = false;
}

void DDMEffect::generate_distance_table(int imax_d) {
    tab_.resize(imax_d);

    std::string pixel_script = parameters().get_string("pixel_script");
    if (!pixel_script.empty()) {
        // Generate distance modification table
        for (int x = 0; x < imax_d - 32; x++) {
            script_engine_.set_variable("d", x / (max_d_ - 1));
            script_engine_.evaluate(pixel_script);
            double d = script_engine_.get_variable("d");
            tab_[x] = static_cast<int>(d * 256.0 * max_d_ / (x + 1));
        }
        // Fill remainder with last value
        for (int x = imax_d - 32; x < imax_d; x++) {
            tab_[x] = tab_[x - 1];
        }
    } else {
        // No pixel script - zero displacement
        std::fill(tab_.begin(), tab_.end(), 0);
    }
}

int DDMEffect::render(AudioData visdata, int isBeat,
                      uint32_t* framebuffer, uint32_t* fbout,
                      int w, int h) {
    uint32_t* fbin = framebuffer;

    // Recalculate on size change
    if (last_h_ != h || last_w_ != w || wmul_.empty()) {
        last_w_ = w;
        last_h_ = h;
        max_d_ = sqrt((w * w + h * h) / 4.0);

        wmul_.resize(h);
        for (int y = 0; y < h; y++) {
            wmul_[y] = y * w;
        }
        tab_.clear();
    }

    int imax_d = static_cast<int>(max_d_ + 32.9);
    if (imax_d < 33) imax_d = 33;

    if (tab_.empty()) {
        tab_.resize(imax_d);
    }

    if (isBeat & 0x80000000) return 0;

    // Set audio context for script engine
    script_engine_.set_audio_context(visdata, isBeat);

    // Set beat variable
    script_engine_.set_variable("b", isBeat ? 1.0 : 0.0);

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

    // Generate distance table using pixel script
    generate_distance_table(imax_d);

    bool blend = parameters().get_bool("blend");
    bool subpixel = parameters().get_bool("bilinear");

    int w2 = w / 2;
    int h2 = h / 2;

    for (int y = 0; y < h; y++) {
        int ty = y - h2;
        int x2 = w2 * w2 + w2 + ty * ty + 256;
        int dx2 = -2 * w2;
        int yysc = ty;
        int xxsc = -w2;

        if (subpixel) {
            // Bilinear filtering path
            for (int x = w; x > 0; x--) {
                int qd = tab_[isqrt(x2)];
                x2 += dx2;
                dx2 += 2;

                int xpart = (qd * xxsc + 128);
                int ypart = (qd * yysc + 128);
                int ow = w2 + (xpart >> 8);
                int oh = h2 + (ypart >> 8);
                xpart &= 0xff;
                ypart &= 0xff;
                xxsc++;

                // Clamp coordinates
                if (ow < 0) ow = 0;
                else if (ow >= w - 1) ow = w - 2;
                if (oh < 0) oh = 0;
                else if (oh >= h - 1) oh = h - 2;

                // Bilinear interpolation (BLEND4 equivalent)
                uint32_t* src = framebuffer + ow + wmul_[oh];
                uint32_t p00 = src[0];
                uint32_t p01 = src[1];
                uint32_t p10 = src[w];
                uint32_t p11 = src[w + 1];

                // Interpolate
                int r = ((((p00 & 0xff) * (256 - xpart) + (p01 & 0xff) * xpart) >> 8) * (256 - ypart) +
                         (((p10 & 0xff) * (256 - xpart) + (p11 & 0xff) * xpart) >> 8) * ypart) >> 8;
                int g = (((((p00 >> 8) & 0xff) * (256 - xpart) + ((p01 >> 8) & 0xff) * xpart) >> 8) * (256 - ypart) +
                         ((((p10 >> 8) & 0xff) * (256 - xpart) + ((p11 >> 8) & 0xff) * xpart) >> 8) * ypart) >> 8;
                int b = (((((p00 >> 16) & 0xff) * (256 - xpart) + ((p01 >> 16) & 0xff) * xpart) >> 8) * (256 - ypart) +
                         ((((p10 >> 16) & 0xff) * (256 - xpart) + ((p11 >> 16) & 0xff) * xpart) >> 8) * ypart) >> 8;

                uint32_t pixel = (b << 16) | (g << 8) | r;

                if (blend) {
                    *fbout++ = BLEND_AVG(pixel, *fbin++);
                } else {
                    *fbout++ = pixel;
                }
            }
        } else {
            // Nearest neighbor path
            for (int x = w; x > 0; x--) {
                int qd = tab_[isqrt(x2)];
                x2 += dx2;
                dx2 += 2;

                int ow = w2 + ((qd * xxsc + 128) >> 8);
                xxsc++;
                int oh = h2 + ((qd * yysc + 128) >> 8);

                // Clamp coordinates
                if (ow < 0) ow = 0;
                else if (ow >= w) ow = w - 1;
                if (oh < 0) oh = 0;
                else if (oh >= h) oh = h - 1;

                if (blend) {
                    *fbout++ = BLEND_AVG(framebuffer[ow + wmul_[oh]], *fbin++);
                } else {
                    *fbout++ = framebuffer[ow + wmul_[oh]];
                }
            }
        }
    }

    return 1;  // Result in fbout
}

// Static member definition - UI layout from res.rc IDD_CFG_DDM
const PluginInfo DDMEffect::effect_info {
    .name = "Dynamic Distance Modifier",
    .category = "Trans",
    .description = "Distance-based distortion with scripted control",
    .author = "",
    .version = 1,
    .legacy_index = 35,  // R_DDM
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<DDMEffect>();
    },
    .ui_layout = {
        {
            // Labels
            {
                .id = "init_label",
                .text = "init",
                .type = ControlType::LABEL,
                .x = 0, .y = 14, .w = 10, .h = 8
            },
            {
                .id = "frame_label",
                .text = "frame",
                .type = ControlType::LABEL,
                .x = 0, .y = 60, .w = 18, .h = 8
            },
            {
                .id = "beat_label",
                .text = "beat",
                .type = ControlType::LABEL,
                .x = 0, .y = 113, .w = 15, .h = 8
            },
            {
                .id = "pixel_label",
                .text = "pixel",
                .type = ControlType::LABEL,
                .x = 0, .y = 166, .w = 15, .h = 8
            },
            // Script edit boxes
            {
                .id = "init_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 0, .w = 208, .h = 39,
                .default_val = "u=1;t=0"
            },
            {
                .id = "frame_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 38, .w = 208, .h = 53,
                .default_val = "t=t+u;t=min(100,t);t=max(0,t);u=if(equal(t,100),-1,u);u=if(equal(t,0),1,u)"
            },
            {
                .id = "beat_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 91, .w = 208, .h = 53,
                .default_val = ""
            },
            {
                .id = "pixel_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 144, .w = 208, .h = 53,
                .default_val = "d=d-sigmoid((t-50)/100,2)"
            },
            // Checkboxes
            {
                .id = "blend",
                .text = "Blend",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 204, .w = 34, .h = 10,
                .default_val = 0
            },
            {
                .id = "bilinear",
                .text = "Bilinear filtering",
                .type = ControlType::CHECKBOX,
                .x = 38, .y = 204, .w = 63, .h = 10,
                .default_val = 0
            }
        }
    }
};

// Register effect at startup
static bool register_ddm = []() {
    PluginManager::instance().register_effect(DDMEffect::effect_info);
    return true;
}();

} // namespace avs
