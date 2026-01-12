// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "mirror.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include <cstring>
#include <cstdlib>

namespace avs {

// Mode flags
constexpr int HORIZONTAL1 = 1;  // Top to bottom
constexpr int HORIZONTAL2 = 2;  // Bottom to top
constexpr int VERTICAL1 = 4;    // Left to right
constexpr int VERTICAL2 = 8;    // Right to left

// Divisor masks and shifts for smooth transitions
constexpr int D_HORIZONTAL1 = 0;
constexpr int D_HORIZONTAL2 = 8;
constexpr int D_VERTICAL1 = 16;
constexpr int D_VERTICAL2 = 24;
constexpr uint32_t M_HORIZONTAL1 = 0x000000FF;
constexpr uint32_t M_HORIZONTAL2 = 0x0000FF00;
constexpr uint32_t M_VERTICAL1 = 0x00FF0000;
constexpr uint32_t M_VERTICAL2 = 0xFF000000;

MirrorEffect::MirrorEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

inline uint32_t MirrorEffect::blend_adapt(uint32_t a, uint32_t b, int divisor) {
    // Blend between two colors based on divisor (0-16)
    return ((((a >> 4) & 0x0F0F0F) * (16 - divisor) +
             (((b >> 4) & 0x0F0F0F) * divisor)));
}

int MirrorEffect::render(AudioData visdata, int isBeat,
                         uint32_t* framebuffer, uint32_t* fbout,
                         int w, int h) {
    if (isBeat & 0x80000000) return 0;
    if (!parameters().get_bool("enabled")) return 0;

    int t = w * h;
    int halfw = w / 2;
    int halfh = h / 2;

    // Build mode from checkboxes
    int mode = 0;
    if (parameters().get_bool("top_to_bottom")) mode |= HORIZONTAL1;
    if (parameters().get_bool("bottom_to_top")) mode |= HORIZONTAL2;
    if (parameters().get_bool("left_to_right")) mode |= VERTICAL1;
    if (parameters().get_bool("right_to_left")) mode |= VERTICAL2;

    bool onbeat = parameters().get_int("mode") == 1;
    bool smooth = parameters().get_bool("smooth");
    int slower = parameters().get_int("slower");

    int* thismode = &mode;

    if (onbeat) {
        if (isBeat) {
            rbeat_ = (rand() % 16) & mode;
        }
        thismode = &rbeat_;
    }

    // Handle mode transitions for smooth effect
    if (*thismode != last_mode_) {
        int dif = *thismode ^ last_mode_;
        for (int i = 1, m = 0xFF, d = 0; i < 16; i <<= 1, m <<= 8, d += 8) {
            if (dif & i) {
                inc_ = (inc_ & ~m) | ((last_mode_ & i) ? 0xFF : 1) << d;
                if (!(divisors_ & m)) {
                    divisors_ = (divisors_ & ~m) | ((last_mode_ & i) ? 16 : 1) << d;
                }
            }
        }
        last_mode_ = *thismode;
    }

    uint32_t* fbp = framebuffer;

    // Left to right (VERTICAL1)
    if ((*thismode & VERTICAL1) || (smooth && (divisors_ & M_VERTICAL1))) {
        int divis = (divisors_ & M_VERTICAL1) >> D_VERTICAL1;
        for (int hi = 0; hi < h; hi++) {
            if (smooth && divis) {
                uint32_t* tmp = fbp + w - 1;
                int n = halfw;
                while (n--) {
                    *tmp-- = blend_adapt(*tmp, *fbp++, divis);
                }
            } else {
                uint32_t* tmp = fbp + w - 1;
                int n = halfw;
                while (n--) {
                    *tmp-- = *fbp++;
                }
            }
            fbp += halfw;
        }
    }

    fbp = framebuffer;

    // Right to left (VERTICAL2)
    if ((*thismode & VERTICAL2) || (smooth && (divisors_ & M_VERTICAL2))) {
        int divis = (divisors_ & M_VERTICAL2) >> D_VERTICAL2;
        for (int hi = 0; hi < h; hi++) {
            if (smooth && divis) {
                uint32_t* tmp = fbp + w - 1;
                int n = halfw;
                while (n--) {
                    *fbp++ = blend_adapt(*fbp, *tmp--, divis);
                }
            } else {
                uint32_t* tmp = fbp + w - 1;
                int n = halfw;
                while (n--) {
                    *fbp++ = *tmp--;
                }
            }
            fbp += halfw;
        }
    }

    fbp = framebuffer;
    int j = t - w;

    // Top to bottom (HORIZONTAL1)
    if ((*thismode & HORIZONTAL1) || (smooth && (divisors_ & M_HORIZONTAL1))) {
        int divis = (divisors_ & M_HORIZONTAL1) >> D_HORIZONTAL1;
        for (int hi = 0; hi < halfh; hi++) {
            if (smooth && divis) {
                int n = w;
                while (n--) {
                    fbp[j] = blend_adapt(fbp[j], *fbp, divis);
                    fbp++;
                }
            } else {
                memcpy(fbp + j, fbp, w * sizeof(uint32_t));
                fbp += w;
            }
            j -= 2 * w;
        }
    }

    fbp = framebuffer;
    j = t - w;

    // Bottom to top (HORIZONTAL2)
    if ((*thismode & HORIZONTAL2) || (smooth && (divisors_ & M_HORIZONTAL2))) {
        int divis = (divisors_ & M_HORIZONTAL2) >> D_HORIZONTAL2;
        for (int hi = 0; hi < halfh; hi++) {
            if (smooth && divis) {
                int n = w;
                while (n--) {
                    *fbp++ = blend_adapt(*fbp, fbp[j], divis);
                }
            } else {
                memcpy(fbp, fbp + j, w * sizeof(uint32_t));
                fbp += w;
            }
            j -= 2 * w;
        }
    }

    // Update smooth transition divisors
    if (smooth && !(++framecount_ % slower)) {
        for (int i = 1, d = 0; i < 16; i <<= 1, d += 8) {
            uint32_t m = static_cast<uint32_t>(0xFF) << d;
            if (divisors_ & m) {
                uint32_t current = (divisors_ & m) >> d;
                uint32_t increment = (inc_ & m) >> d;
                divisors_ = (divisors_ & ~m) | (((current + increment) % 16) << d);
            }
        }
    }

    return 0;  // Rendered in place
}

void MirrorEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_mirror.cpp:
    // enabled (int32), mode (int32), onbeat (int32), smooth (int32), slower (int32)

    if (reader.remaining() >= 4) {
        parameters().set_bool("enabled", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        int mode = reader.read_u32();
        parameters().set_bool("top_to_bottom", (mode & HORIZONTAL1) != 0);
        parameters().set_bool("bottom_to_top", (mode & HORIZONTAL2) != 0);
        parameters().set_bool("left_to_right", (mode & VERTICAL1) != 0);
        parameters().set_bool("right_to_left", (mode & VERTICAL2) != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("mode", reader.read_u32() != 0 ? 1 : 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_bool("smooth", reader.read_u32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("slower", reader.read_u32());
    }
}

// Static member definition
const PluginInfo MirrorEffect::effect_info {
    .name = "Mirror",
    .category = "Trans",
    .description = "Mirror/flip transformations",
    .author = "",
    .version = 1,
    .legacy_index = 26,  // R_Mirror
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<MirrorEffect>();
    },
    .ui_layout = {
        {
            // From res.rc IDD_CFG_MIRROR
            {
                .id = "enabled",
                .text = "Enable Mirror effect",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 77, .h = 10,
                .default_val = true
            },
            {
                .id = "mode",
                .text = "",
                .type = ControlType::RADIO_GROUP,
                .x = 0, .y = 20, .w = 65, .h = 20,
                .default_val = 0,
                .radio_options = {
                    {"Static", 0, 0, 34, 10},
                    {"OnBeat random", 0, 9, 65, 10}
                }
            },
            {
                .id = "groupbox",
                .text = "",
                .type = ControlType::GROUPBOX,
                .x = 0, .y = 36, .w = 137, .h = 51
            },
            {
                .id = "top_to_bottom",
                .text = "Copy Top to Bottom",
                .type = ControlType::CHECKBOX,
                .x = 3, .y = 44, .w = 79, .h = 10,
                .default_val = true
            },
            {
                .id = "bottom_to_top",
                .text = "Copy Bottom to Top",
                .type = ControlType::CHECKBOX,
                .x = 3, .y = 54, .w = 88, .h = 9,
                .default_val = 0
            },
            {
                .id = "left_to_right",
                .text = "Copy Left to Right",
                .type = ControlType::CHECKBOX,
                .x = 3, .y = 63, .w = 88, .h = 10,
                .default_val = 0
            },
            {
                .id = "right_to_left",
                .text = "Copy Right to Left",
                .type = ControlType::CHECKBOX,
                .x = 3, .y = 73, .w = 98, .h = 10,
                .default_val = 0
            },
            {
                .id = "smooth",
                .text = "Smooth transitions",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 90, .w = 73, .h = 10,
                .default_val = 0
            },
            {
                .id = "faster_label",
                .text = "Faster",
                .type = ControlType::LABEL,
                .x = 0, .y = 114, .w = 20, .h = 8
            },
            {
                .id = "slower",
                .text = "",
                .type = ControlType::SLIDER,
                .x = 0, .y = 103, .w = 137, .h = 11,
                .range = {1, 16},
                .default_val = 4
            },
            {
                .id = "slower_label",
                .text = "Slower",
                .type = ControlType::LABEL,
                .x = 115, .y = 114, .w = 22, .h = 8
            }
        }
    }
};

// Register effect at startup
static bool register_mirror = []() {
    PluginManager::instance().register_effect(MirrorEffect::effect_info);
    return true;
}();

} // namespace avs
