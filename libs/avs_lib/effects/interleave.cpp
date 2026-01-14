// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "interleave.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include <algorithm>

namespace avs {

InterleaveEffect::InterleaveEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

int InterleaveEffect::render(AudioData visdata, int isBeat,
                              uint32_t* framebuffer, uint32_t* fbout,
                              int w, int h) {
    if (isBeat & 0x80000000) return 0;

    int enabled = parameters().get_int("enabled");
    if (!enabled) return 0;

    int x_spacing = parameters().get_int("x");
    int y_spacing = parameters().get_int("y");
    int x2 = parameters().get_int("x2");
    int y2 = parameters().get_int("y2");
    int beatdur = parameters().get_int("beatdur");
    int onbeat = parameters().get_int("onbeat");
    uint32_t color = parameters().get_color("color");
    int blend_mode = parameters().get_int("blend");
    int blendavg = parameters().get_int("blendavg");

    // Smooth interpolation toward target values
    double sc1 = (beatdur + 512.0 - 64.0) / 512.0;
    cur_x_ = cur_x_ * sc1 + x_spacing * (1.0 - sc1);
    cur_y_ = cur_y_ * sc1 + y_spacing * (1.0 - sc1);

    // On beat, jump to beat values
    if (isBeat && onbeat) {
        cur_x_ = x2;
        cur_y_ = y2;
    }

    int tx = static_cast<int>(cur_x_);
    int ty = static_cast<int>(cur_y_);

    if (tx < 0 || ty < 0) return 0;

    int ystat = 0;
    int yp = 0;
    int xos = 0;

    if (!ty) {
        ystat = 1;
    }
    if (tx > 0) {
        xos = (w % tx) / 2;
    }
    if (ty > 0) {
        yp = (h % ty) / 2;
    }

    uint32_t* p = framebuffer;

    for (int j = 0; j < h; j++) {
        int xstat = 0;

        if (ty && ++yp >= ty) {
            ystat = !ystat;
            yp = 0;
        }

        int l = w;

        if (!ystat) {
            // This line is pure color
            if (blend_mode) {
                while (l--) {
                    *p = BLEND(*p, color);
                    p++;
                }
            } else if (blendavg) {
                while (l--) {
                    *p = BLEND_AVG(*p, color);
                    p++;
                }
            } else {
                while (l--) {
                    *p++ = color;
                }
            }
        } else if (tx) {
            // Interleaved pattern on this line
            if (blend_mode) {
                int xo = xos;
                while (l > 0) {
                    int l2 = std::min(l, tx - xo);
                    xo = 0;
                    l -= l2;
                    if (xstat) {
                        p += l2;
                    } else {
                        while (l2--) {
                            *p = BLEND(*p, color);
                            p++;
                        }
                    }
                    xstat = !xstat;
                }
            } else if (blendavg) {
                int xo = xos;
                while (l > 0) {
                    int l2 = std::min(l, tx - xo);
                    xo = 0;
                    l -= l2;
                    if (xstat) {
                        p += l2;
                    } else {
                        while (l2--) {
                            *p = BLEND_AVG(*p, color);
                            p++;
                        }
                    }
                    xstat = !xstat;
                }
            } else {
                int xo = xos;
                while (l > 0) {
                    int l2 = std::min(l, tx - xo);
                    xo = 0;
                    l -= l2;
                    if (xstat) {
                        p += l2;
                    } else {
                        while (l2--) {
                            *p++ = color;
                        }
                    }
                    xstat = !xstat;
                }
            }
        } else {
            p += w;
        }
    }

    return 0;
}

void InterleaveEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;

    BinaryReader reader(data);

    // Binary format from r_interleave.cpp:
    // enabled, x, y, color, blend, blendavg, onbeat, x2, y2, beatdur
    int enabled = reader.read_u32();
    parameters().set_int("enabled", enabled);

    if (reader.remaining() >= 4) {
        int x = reader.read_u32();
        parameters().set_int("x", x);
        cur_x_ = x;
    }

    if (reader.remaining() >= 4) {
        int y = reader.read_u32();
        parameters().set_int("y", y);
        cur_y_ = y;
    }

    if (reader.remaining() >= 4) {
        uint32_t color = BinaryReader::bgr_add_alpha(reader.read_u32());
        parameters().set_color("color", color);
    }

    if (reader.remaining() >= 4) {
        int blend = reader.read_u32();
        parameters().set_int("blend", blend);
    }

    if (reader.remaining() >= 4) {
        int blendavg = reader.read_u32();
        parameters().set_int("blendavg", blendavg);
    }

    if (reader.remaining() >= 4) {
        int onbeat = reader.read_u32();
        parameters().set_int("onbeat", onbeat);
    }

    if (reader.remaining() >= 4) {
        int x2 = reader.read_u32();
        parameters().set_int("x2", x2);
    }

    if (reader.remaining() >= 4) {
        int y2 = reader.read_u32();
        parameters().set_int("y2", y2);
    }

    if (reader.remaining() >= 4) {
        int beatdur = reader.read_u32();
        parameters().set_int("beatdur", beatdur);
    }
}

// Static member definition - UI layout from res.rc IDD_CFG_INTERLEAVE
const PluginInfo InterleaveEffect::effect_info {
    .name = "Interleave",
    .category = "Trans",
    .description = "Draws colored grid pattern over the image",
    .author = "",
    .version = 1,
    .legacy_index = 24,  // R_Interleave from rlib.cpp
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<InterleaveEffect>();
    },
    .ui_layout = {
        {
            {
                .id = "enabled",
                .text = "Enable",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 78, .h = 10,
                .default_val = 1
            },
            {
                .id = "x",
                .text = "X spacing",
                .type = ControlType::SLIDER,
                .x = 0, .y = 15, .w = 150, .h = 20,
                .range = {0, 64, 1},
                .default_val = 1
            },
            {
                .id = "y",
                .text = "Y spacing",
                .type = ControlType::SLIDER,
                .x = 0, .y = 40, .w = 150, .h = 20,
                .range = {0, 64, 1},
                .default_val = 1
            },
            {
                .id = "color",
                .text = "Color",
                .type = ControlType::COLOR_BUTTON,
                .x = 0, .y = 65, .w = 78, .h = 20,
                .default_val = 0  // Black
            },
            {
                .id = "onbeat",
                .text = "On beat",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 90, .w = 78, .h = 10,
                .default_val = 0
            },
            {
                .id = "x2",
                .text = "Beat X",
                .type = ControlType::SLIDER,
                .x = 0, .y = 105, .w = 150, .h = 20,
                .range = {0, 64, 1},
                .default_val = 1
            },
            {
                .id = "y2",
                .text = "Beat Y",
                .type = ControlType::SLIDER,
                .x = 0, .y = 130, .w = 150, .h = 20,
                .range = {0, 64, 1},
                .default_val = 1
            },
            {
                .id = "beatdur",
                .text = "Duration",
                .type = ControlType::SLIDER,
                .x = 0, .y = 155, .w = 150, .h = 20,
                .range = {1, 64, 1},
                .default_val = 4
            },
            {
                .id = "blend",
                .text = "Additive",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 180, .w = 78, .h = 10,
                .default_val = 0
            },
            {
                .id = "blendavg",
                .text = "50/50",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 195, .w = 78, .h = 10,
                .default_val = 0
            }
        }
    }
};

// Register effect at startup
static bool register_interleave = []() {
    PluginManager::instance().register_effect(InterleaveEffect::effect_info);
    return true;
}();

} // namespace avs
