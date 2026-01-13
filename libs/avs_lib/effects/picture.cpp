// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Port of r_picture.cpp from original AVS

#include "picture.h"
#include "core/plugin_manager.h"
#include "core/binary_reader.h"
#include "core/blend.h"
#include "third_party/stb_image.h"
#include <algorithm>
#include <cstring>

namespace avs {

PictureEffect::PictureEffect() {
    init_parameters_from_layout(effect_info.ui_layout);
}

PictureEffect::~PictureEffect() {
    free_image();
}

void PictureEffect::load_image(const std::string& filename) {
    free_image();

    if (filename.empty()) return;

    // Combine resource_path with filename
    std::string full_path = resource_path();
    if (!full_path.empty() && full_path.back() != '/' && full_path.back() != '\\') {
        full_path += '/';
    }
    full_path += filename;

    int w, h, channels;
    unsigned char* data = stbi_load(full_path.c_str(), &w, &h, &channels, 4);

    if (!data) return;

    image_width_ = w;
    image_height_ = h;
    image_data_.resize(w * h);

    // Convert RGBA to our format (0xAABBGGRR - alpha, blue, green, red)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            int src_idx = idx * 4;
            uint8_t r = data[src_idx + 0];
            uint8_t g = data[src_idx + 1];
            uint8_t b = data[src_idx + 2];
            uint8_t a = data[src_idx + 3];
            image_data_[idx] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    }

    stbi_image_free(data);

    // Force rescale on next render
    last_width_ = 0;
    last_height_ = 0;
}

void PictureEffect::free_image() {
    image_data_.clear();
    scaled_data_.clear();
    image_width_ = 0;
    image_height_ = 0;
    last_width_ = 0;
    last_height_ = 0;
}

void PictureEffect::update_scaled_image(int w, int h) {
    if (w == last_width_ && h == last_height_) return;
    if (image_width_ == 0 || image_height_ == 0) return;

    last_width_ = w;
    last_height_ = h;
    scaled_data_.resize(w * h);

    // Calculate destination rectangle based on aspect ratio settings
    bool keep_ratio = parameters().get_bool("ratio");
    int axis_ratio = parameters().get_int("axis_ratio");

    int final_width = w;
    int final_height = h;
    int start_x = 0;
    int start_y = 0;

    if (keep_ratio) {
        if (axis_ratio == 0) {
            // Ratio on X axis - scale height proportionally
            final_height = image_height_ * w / image_width_;
            start_y = (h - final_height) / 2;
        } else {
            // Ratio on Y axis - scale width proportionally
            final_width = image_width_ * h / image_height_;
            start_x = (w - final_width) / 2;
        }
    }

    // Clear to black
    std::fill(scaled_data_.begin(), scaled_data_.end(), 0);

    // Bilinear-ish scaling (simple nearest neighbor for now, matching original)
    for (int y = 0; y < final_height; y++) {
        int dst_y = start_y + y;
        if (dst_y < 0 || dst_y >= h) continue;

        int src_y = (y * image_height_) / final_height;
        if (src_y >= image_height_) src_y = image_height_ - 1;

        for (int x = 0; x < final_width; x++) {
            int dst_x = start_x + x;
            if (dst_x < 0 || dst_x >= w) continue;

            int src_x = (x * image_width_) / final_width;
            if (src_x >= image_width_) src_x = image_width_ - 1;

            scaled_data_[dst_y * w + dst_x] = image_data_[src_y * image_width_ + src_x];
        }
    }
}

int PictureEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (!parameters().get_bool("enabled")) return 0;
    if (image_width_ == 0 || image_height_ == 0) return 0;

    // Update scaled image if size changed
    update_scaled_image(w, h);

    if (isBeat & 0x80000000) return 0;

    // Get blend parameters
    int blend_mode = parameters().get_int("blend_mode");
    int persist = parameters().get_int("persist");

    // Handle beat persistence
    if (isBeat)
        persist_count_ = persist;
    else if (persist_count_ > 0)
        persist_count_--;

    // Blend modes:
    // 0 = Replace
    // 1 = Additive
    // 2 = 50/50
    // 3 = Adapt (50/50 normally, additive on beat)
    bool use_additive = (blend_mode == 1) ||
                        (blend_mode == 3 && (isBeat || persist_count_ > 0));
    bool use_5050 = (blend_mode == 2) || (blend_mode == 3 && !use_additive);

    // Apply image to framebuffer
    // Original flips Y during copy, but we already flipped during load
    for (int i = 0; i < w * h; i++) {
        uint32_t src = scaled_data_[i];
        if (use_additive) {
            framebuffer[i] = BLEND(src, framebuffer[i]);
        } else if (use_5050) {
            framebuffer[i] = BLEND_AVG(src, framebuffer[i]);
        } else {
            framebuffer[i] = src;
        }
    }

    return 0;
}

void PictureEffect::on_parameter_changed(const std::string& name) {
    if (name == "filename") {
        load_image(parameters().get_string("filename"));
    } else if (name == "ratio" || name == "axis_ratio") {
        // Force rescale
        last_width_ = 0;
        last_height_ = 0;
    }
}

void PictureEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Binary format from r_picture.cpp:
    // enabled (int32)
    // blend (int32)
    // blendavg (int32)
    // adapt (int32)
    // persist (int32)
    // filename (null-terminated string)
    // ratio (int32)
    // axis_ratio (int32)

    if (reader.remaining() >= 4) {
        parameters().set_bool("enabled", reader.read_i32() != 0);
    }

    int blend = 0, blendavg = 0, adapt = 0;
    if (reader.remaining() >= 4) blend = reader.read_i32();
    if (reader.remaining() >= 4) blendavg = reader.read_i32();
    if (reader.remaining() >= 4) adapt = reader.read_i32();

    // Convert to unified blend_mode
    if (adapt) {
        parameters().set_int("blend_mode", 3);
    } else if (blend) {
        parameters().set_int("blend_mode", 1);
    } else if (blendavg) {
        parameters().set_int("blend_mode", 2);
    } else {
        parameters().set_int("blend_mode", 0);
    }

    if (reader.remaining() >= 4) {
        parameters().set_int("persist", reader.read_i32());
    }

    // Read null-terminated filename
    std::string filename;
    while (reader.remaining() > 0) {
        uint8_t c = reader.read_u8();
        if (c == 0) break;
        filename += static_cast<char>(c);
    }
    parameters().set_string("filename", filename);

    if (reader.remaining() >= 4) {
        parameters().set_bool("ratio", reader.read_i32() != 0);
    }
    if (reader.remaining() >= 4) {
        parameters().set_int("axis_ratio", reader.read_i32());
    }

    // Load the image
    if (!filename.empty()) {
        load_image(filename);
    }
}

// UI controls from res.rc IDD_CFG_PICTURE
static const std::vector<ControlLayout> ui_controls = {
    {
        .id = "enabled",
        .text = "Enable Picture rendering",
        .type = ControlType::CHECKBOX,
        .x = 0, .y = 0, .w = 93, .h = 10,
        .default_val = true
    },
    {
        .id = "filename",
        .text = "*.bmp;*.png;*.jpg;*.jpeg;*.gif;*.tga;*.psd;*.hdr",
        .type = ControlType::FILE_DROPDOWN,
        .x = 1, .y = 14, .w = 232, .h = 100
    },
    {
        .id = "blend_mode",
        .text = "",
        .type = ControlType::RADIO_GROUP,
        .x = 0, .y = 30, .w = 120, .h = 44,
        .default_val = 2,  // 50/50 blend
        .radio_options = {
            {.label = "Replace", .x = 0, .y = 0},
            {.label = "Additive blend", .x = 0, .y = 11},
            {.label = "Blend 50/50", .x = 0, .y = 22},
            {.label = "50/50 + OnBeat Additive", .x = 0, .y = 33}
        }
    },
    {
        .id = "persist_group",
        .text = "Beat persistence",
        .type = ControlType::GROUPBOX,
        .x = 0, .y = 93, .w = 137, .h = 42
    },
    {
        .id = "fast_label",
        .text = "Fast",
        .type = ControlType::LABEL,
        .x = 4, .y = 103, .w = 14, .h = 8
    },
    {
        .id = "long_label",
        .text = "Long",
        .type = ControlType::LABEL,
        .x = 115, .y = 103, .w = 17, .h = 8
    },
    {
        .id = "persist",
        .text = "",
        .type = ControlType::SLIDER,
        .x = 6, .y = 112, .w = 125, .h = 19,
        .range = {0, 32},
        .default_val = 6
    },
    {
        .id = "ratio",
        .text = "Keep aspect ratio",
        .type = ControlType::CHECKBOX,
        .x = 2, .y = 141, .w = 71, .h = 10,
        .default_val = false
    },
    {
        .id = "axis_ratio",
        .text = "",
        .type = ControlType::RADIO_GROUP,
        .x = 20, .y = 156, .w = 50, .h = 22,
        .default_val = 0,
        .radio_options = {
            {.label = "On X-Axis", .x = 0, .y = 0},
            {.label = "On Y-Axis", .x = 0, .y = 11}
        }
    }
};

const PluginInfo PictureEffect::effect_info{
    .name = "Picture",
    .category = "Render",
    .description = "Display image files (JPEG, PNG, BMP, GIF, TGA)",
    .author = "",
    .version = 1,
    .legacy_index = 8,  // R_Picture
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<PictureEffect>();
    },
    .ui_layout = {ui_controls},
    .help_text = ""
};

// Register effect at startup
static bool register_picture = []() {
    PluginManager::instance().register_effect(PictureEffect::effect_info);
    return true;
}();

} // namespace avs
