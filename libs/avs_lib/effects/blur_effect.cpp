// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "blur_effect.h"
#include "core/plugin_manager.h"
#include "core/ui.h"

namespace avs {

// Bit masks for fast division without overflow between color channels
// These masks clear the bits that would overflow into adjacent channels when shifting
#define MASK_SH1 (~(((1<<7)|(1<<15)|(1<<23))<<1))   // For /2
#define MASK_SH2 (~(((3<<6)|(3<<14)|(3<<22))<<2))   // For /4
#define MASK_SH3 (~(((7<<5)|(7<<13)|(7<<21))<<3))   // For /8
#define MASK_SH4 (~(((15<<4)|(15<<12)|(15<<20))<<4)) // For /16

// Fast division macros using bit shifts (no actual division)
#define DIV_2(x)  (((x) & MASK_SH1) >> 1)
#define DIV_4(x)  (((x) & MASK_SH2) >> 2)
#define DIV_8(x)  (((x) & MASK_SH3) >> 3)
#define DIV_16(x) (((x) & MASK_SH4) >> 4)

BlurEffect::BlurEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int BlurEffect::render(AudioData visdata, int isBeat,
                      uint32_t* framebuffer, uint32_t* fbout,
                      int w, int h) {
    if (isBeat & 0x80000000) return 0;

    auto blur_level = static_cast<BlurLevel>(parameters().get_int("blur_level"));
    if (blur_level == BlurLevel::NONE) return 0;

    auto round_mode = static_cast<RoundMode>(parameters().get_int("round_mode"));
    bool round_up = (round_mode == RoundMode::UP);

    unsigned int* f = (unsigned int*)framebuffer;
    unsigned int* of = (unsigned int*)fbout;

    if (blur_level == BlurLevel::LIGHT) {
        // Light blur: 3/4 center + 1/16 each neighbor
        // Top line
        {
            unsigned int* f2 = f + w;
            unsigned int adj_tl = round_up ? 0x03030303 : 0;
            unsigned int adj_tl2 = round_up ? 0x04040404 : 0;

            // Top left corner
            *of++ = DIV_2(f[0]) + DIV_4(f[0]) + DIV_8(f[1]) + DIV_8(f2[0]) + adj_tl;
            f++; f2++;

            // Top center
            for (int x = 0; x < w - 2; x++) {
                *of++ = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[1]) + DIV_8(f[-1]) + DIV_8(f2[0]) + adj_tl2;
                f++; f2++;
            }

            // Top right corner
            *of++ = DIV_2(f[0]) + DIV_4(f[0]) + DIV_8(f[-1]) + DIV_8(f2[0]) + adj_tl;
            f++; f2++;
        }

        // Middle rows
        {
            unsigned int adj_tl1 = round_up ? 0x04040404 : 0;
            unsigned int adj_tl2 = round_up ? 0x05050505 : 0;

            for (int y = 1; y < h - 1; y++) {
                unsigned int* f2 = f + w;
                unsigned int* f3 = f - w;

                // Left edge
                *of++ = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[1]) + DIV_8(f2[0]) + DIV_8(f3[0]) + adj_tl1;
                f++; f2++; f3++;

                // Middle of line - process 4 pixels at a time for speed
                int x = (w - 2) / 4;
                while (x--) {
                    of[0] = DIV_2(f[0]) + DIV_4(f[0]) + DIV_16(f[1]) + DIV_16(f[-1]) + DIV_16(f2[0]) + DIV_16(f3[0]) + adj_tl2;
                    of[1] = DIV_2(f[1]) + DIV_4(f[1]) + DIV_16(f[2]) + DIV_16(f[0]) + DIV_16(f2[1]) + DIV_16(f3[1]) + adj_tl2;
                    of[2] = DIV_2(f[2]) + DIV_4(f[2]) + DIV_16(f[3]) + DIV_16(f[1]) + DIV_16(f2[2]) + DIV_16(f3[2]) + adj_tl2;
                    of[3] = DIV_2(f[3]) + DIV_4(f[3]) + DIV_16(f[4]) + DIV_16(f[2]) + DIV_16(f2[3]) + DIV_16(f3[3]) + adj_tl2;
                    f += 4; f2 += 4; f3 += 4; of += 4;
                }

                // Remaining pixels
                x = (w - 2) & 3;
                while (x--) {
                    *of++ = DIV_2(f[0]) + DIV_4(f[0]) + DIV_16(f[1]) + DIV_16(f[-1]) + DIV_16(f2[0]) + DIV_16(f3[0]) + adj_tl2;
                    f++; f2++; f3++;
                }

                // Right edge
                *of++ = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[-1]) + DIV_8(f2[0]) + DIV_8(f3[0]) + adj_tl1;
                f++;
            }
        }

        // Bottom line
        {
            unsigned int* f2 = f - w;
            unsigned int adj_tl = round_up ? 0x03030303 : 0;
            unsigned int adj_tl2 = round_up ? 0x04040404 : 0;

            // Bottom left
            *of++ = DIV_2(f[0]) + DIV_4(f[0]) + DIV_8(f[1]) + DIV_8(f2[0]) + adj_tl;
            f++; f2++;

            // Bottom center
            for (int x = 0; x < w - 2; x++) {
                *of++ = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[1]) + DIV_8(f[-1]) + DIV_8(f2[0]) + adj_tl2;
                f++; f2++;
            }

            // Bottom right
            *of++ = DIV_2(f[0]) + DIV_4(f[0]) + DIV_8(f[-1]) + DIV_8(f2[0]) + adj_tl;
        }
    }
    else if (blur_level == BlurLevel::HEAVY) {
        // Heavy blur: 0 center + 1/4 each neighbor (excludes center pixel)
        // Top line
        {
            unsigned int* f2 = f + w;
            unsigned int adj_tl = round_up ? 0x02020202 : 0;
            unsigned int adj_tl2 = round_up ? 0x01010101 : 0;

            // Top left
            *of++ = DIV_2(f[1]) + DIV_2(f2[0]) + adj_tl2;
            f++; f2++;

            // Top center
            for (int x = 0; x < w - 2; x++) {
                *of++ = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_2(f2[0]) + adj_tl;
                f++; f2++;
            }

            // Top right
            *of++ = DIV_2(f[-1]) + DIV_2(f2[0]) + adj_tl2;
            f++; f2++;
        }

        // Middle rows
        {
            unsigned int adj_tl1 = round_up ? 0x02020202 : 0;
            unsigned int adj_tl2 = round_up ? 0x03030303 : 0;

            for (int y = 1; y < h - 1; y++) {
                unsigned int* f2 = f + w;
                unsigned int* f3 = f - w;

                // Left edge
                *of++ = DIV_2(f[1]) + DIV_4(f2[0]) + DIV_4(f3[0]) + adj_tl1;
                f++; f2++; f3++;

                // Middle - 4 pixels at a time
                int x = (w - 2) / 4;
                while (x--) {
                    of[0] = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f2[0]) + DIV_4(f3[0]) + adj_tl2;
                    of[1] = DIV_4(f[2]) + DIV_4(f[0]) + DIV_4(f2[1]) + DIV_4(f3[1]) + adj_tl2;
                    of[2] = DIV_4(f[3]) + DIV_4(f[1]) + DIV_4(f2[2]) + DIV_4(f3[2]) + adj_tl2;
                    of[3] = DIV_4(f[4]) + DIV_4(f[2]) + DIV_4(f2[3]) + DIV_4(f3[3]) + adj_tl2;
                    f += 4; f2 += 4; f3 += 4; of += 4;
                }

                x = (w - 2) & 3;
                while (x--) {
                    *of++ = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f2[0]) + DIV_4(f3[0]) + adj_tl2;
                    f++; f2++; f3++;
                }

                // Right edge
                *of++ = DIV_2(f[-1]) + DIV_4(f2[0]) + DIV_4(f3[0]) + adj_tl1;
                f++;
            }
        }

        // Bottom line
        {
            unsigned int* f2 = f - w;
            unsigned int adj_tl = round_up ? 0x02020202 : 0;
            unsigned int adj_tl2 = round_up ? 0x01010101 : 0;

            // Bottom left
            *of++ = DIV_2(f[1]) + DIV_2(f2[0]) + adj_tl2;
            f++; f2++;

            // Bottom center
            for (int x = 0; x < w - 2; x++) {
                *of++ = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_2(f2[0]) + adj_tl;
                f++; f2++;
            }

            // Bottom right
            *of++ = DIV_2(f[-1]) + DIV_2(f2[0]) + adj_tl2;
        }
    }
    else {
        // Medium blur (default): 1/2 center + 1/8 each neighbor
        // Top line
        {
            unsigned int* f2 = f + w;
            unsigned int adj_tl = round_up ? 0x02020202 : 0;
            unsigned int adj_tl2 = round_up ? 0x03030303 : 0;

            // Top left
            *of++ = DIV_2(f[0]) + DIV_4(f[1]) + DIV_4(f2[0]) + adj_tl;
            f++; f2++;

            // Top center
            for (int x = 0; x < w - 2; x++) {
                *of++ = DIV_4(f[0]) + DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f2[0]) + adj_tl2;
                f++; f2++;
            }

            // Top right
            *of++ = DIV_2(f[0]) + DIV_4(f[-1]) + DIV_4(f2[0]) + adj_tl;
            f++; f2++;
        }

        // Middle rows
        {
            unsigned int adj_tl1 = round_up ? 0x03030303 : 0;
            unsigned int adj_tl2 = round_up ? 0x04040404 : 0;

            for (int y = 1; y < h - 1; y++) {
                unsigned int* f2 = f + w;
                unsigned int* f3 = f - w;

                // Left edge
                *of++ = DIV_4(f[0]) + DIV_4(f[1]) + DIV_4(f2[0]) + DIV_4(f3[0]) + adj_tl1;
                f++; f2++; f3++;

                // Middle - 4 pixels at a time
                int x = (w - 2) / 4;
                while (x--) {
                    of[0] = DIV_2(f[0]) + DIV_8(f[1]) + DIV_8(f[-1]) + DIV_8(f2[0]) + DIV_8(f3[0]) + adj_tl2;
                    of[1] = DIV_2(f[1]) + DIV_8(f[2]) + DIV_8(f[0]) + DIV_8(f2[1]) + DIV_8(f3[1]) + adj_tl2;
                    of[2] = DIV_2(f[2]) + DIV_8(f[3]) + DIV_8(f[1]) + DIV_8(f2[2]) + DIV_8(f3[2]) + adj_tl2;
                    of[3] = DIV_2(f[3]) + DIV_8(f[4]) + DIV_8(f[2]) + DIV_8(f2[3]) + DIV_8(f3[3]) + adj_tl2;
                    f += 4; f2 += 4; f3 += 4; of += 4;
                }

                x = (w - 2) & 3;
                while (x--) {
                    *of++ = DIV_2(f[0]) + DIV_8(f[1]) + DIV_8(f[-1]) + DIV_8(f2[0]) + DIV_8(f3[0]) + adj_tl2;
                    f++; f2++; f3++;
                }

                // Right edge
                *of++ = DIV_4(f[0]) + DIV_4(f[-1]) + DIV_4(f2[0]) + DIV_4(f3[0]) + adj_tl1;
                f++;
            }
        }

        // Bottom line
        {
            unsigned int* f2 = f - w;
            unsigned int adj_tl = round_up ? 0x02020202 : 0;
            unsigned int adj_tl2 = round_up ? 0x03030303 : 0;

            // Bottom left
            *of++ = DIV_2(f[0]) + DIV_4(f[1]) + DIV_4(f2[0]) + adj_tl;
            f++; f2++;

            // Bottom center
            for (int x = 0; x < w - 2; x++) {
                *of++ = DIV_4(f[0]) + DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f2[0]) + adj_tl2;
                f++; f2++;
            }

            // Bottom right
            *of++ = DIV_2(f[0]) + DIV_4(f[-1]) + DIV_4(f2[0]) + adj_tl;
        }
    }

    return 1;  // Use output buffer
}

// Static member definition
const PluginInfo BlurEffect::effect_info {
    .name = "Blur",
    .category = "Trans",
    .description = "",
    .author = "",
    .version = 1,
    .legacy_index = 6,  // R_Blur
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<BlurEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "blur_level",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"No blur", 2, 1, 39, 10},
                    {"Light blur", 2, 12, 45, 10},
                    {"Medium blur", 2, 23, 54, 10},
                    {"Heavy blur", 2, 34, 50, 10}
                },
                .default_val = static_cast<int>(BlurLevel::MEDIUM)
            },
            {
                .id = "round_mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Round down", 3, 58, 57, 10},
                    {"Round up", 3, 69, 47, 10}
                },
                .default_val = static_cast<int>(RoundMode::DOWN)
            }
        }
    }
};

// Register effect at startup
static bool register_blur = []() {
    PluginManager::instance().register_effect(BlurEffect::effect_info);
    return true;
}();

} // namespace avs
