// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "dynamic_movement_effect.h"
#include "core/parameter.h"
#include "core/plugin_manager.h"
#include "core/ui.h"
#include <cmath>
#include <cstring>

namespace avs {

DynamicMovementEffect::DynamicMovementEffect()
    : last_width_(0), last_height_(0), last_grid_width_(0), last_grid_height_(0),
      last_rectangular_(false), last_subpixel_(true),
      last_wrap_(false), last_blend_(false), last_buffer_source_(0), script_initialized_(false)
{
    // Initialize script variables
    memset(script_vars_, 0, sizeof(script_vars_));
    init_parameters_from_layout(effect_info.ui_layout);

    // Set default script values (STRING parameters need manual init)
    parameters().add_parameter(std::make_shared<Parameter>("init_script", ParameterType::STRING,
        std::string("")));
    parameters().add_parameter(std::make_shared<Parameter>("frame_script", ParameterType::STRING,
        std::string("")));
    parameters().add_parameter(std::make_shared<Parameter>("beat_script", ParameterType::STRING,
        std::string("")));
    parameters().add_parameter(std::make_shared<Parameter>("pixel_script", ParameterType::STRING,
        std::string("d=d*0.9")));
}

bool DynamicMovementEffect::needs_grid_regeneration(int w, int h, AudioData visdata) const {
    int grid_width = parameters().get_int("grid_width", 16);
    int grid_height = parameters().get_int("grid_height", 16);
    std::string init_script = parameters().get_string("init_script");
    std::string frame_script = parameters().get_string("frame_script");
    std::string beat_script = parameters().get_string("beat_script");
    std::string pixel_script = parameters().get_string("pixel_script");
    bool rectangular = parameters().get_bool("rectangular", false);
    // "bilinear" parameter controls subpixel sampling from source image
    bool subpixel = parameters().get_bool("bilinear", true);
    bool wrap = parameters().get_bool("wrap", false);
    bool blend = parameters().get_bool("blend", false);

    return w != last_width_ ||
           h != last_height_ ||
           grid_width != last_grid_width_ ||
           grid_height != last_grid_height_ ||
           init_script != last_init_script_ ||
           frame_script != last_frame_script_ ||
           beat_script != last_beat_script_ ||
           pixel_script != last_pixel_script_ ||
           rectangular != last_rectangular_ ||
           subpixel != last_subpixel_ ||
           wrap != last_wrap_ ||
           blend != last_blend_;
}

void DynamicMovementEffect::generate_grid(int w, int h, AudioData visdata, int isBeat) {
    int grid_width = parameters().get_int("grid_width", 16);
    int grid_height = parameters().get_int("grid_height", 16);
    bool rectangular = parameters().get_bool("rectangular", false);
    // "bilinear" parameter controls subpixel sampling from source image
    bool subpixel = parameters().get_bool("bilinear", true);
    bool wrap = parameters().get_bool("wrap", false);
    std::string pixel_script = parameters().get_string("pixel_script");

    // Execute script phases in order
    execute_init_script(visdata, w, h);
    execute_frame_script(visdata, w, h);
    if (isBeat) {
        execute_beat_script(visdata, w, h);
    }

    // Generate grid using CoordinateGrid
    // The grid handles script evaluation internally
    CoordMode mode = rectangular ? CoordMode::RECTANGULAR : CoordMode::POLAR;
    grid_.generate(grid_width, grid_height, w, h, pixel_script, mode, visdata);

    // Update state
    last_width_ = w;
    last_height_ = h;
    last_grid_width_ = grid_width;
    last_grid_height_ = grid_height;
    last_init_script_ = parameters().get_string("init_script");
    last_frame_script_ = parameters().get_string("frame_script");
    last_beat_script_ = parameters().get_string("beat_script");
    last_pixel_script_ = pixel_script;
    last_rectangular_ = rectangular;
    last_subpixel_ = subpixel;
    last_wrap_ = wrap;
    last_blend_ = parameters().get_bool("blend", false);
}



int DynamicMovementEffect::render(AudioData visdata, int isBeat,
                                 uint32_t* framebuffer, uint32_t* fbout,
                                 int w, int h) {
    if (!is_enabled()) return 0;

    bool no_movement = parameters().get_bool("no_movement", false);
    if (no_movement) {
        // Just copy input to output
        memcpy(fbout, framebuffer, w * h * sizeof(uint32_t));
        return 1;
    }

    // Check if we need to regenerate the grid
    if (needs_grid_regeneration(w, h, visdata)) {
        generate_grid(w, h, visdata, isBeat);
    }

    // Apply grid transformation
    // subpixel controls bilinear sampling from source image
    // Grid interpolation is always bilinear (matching original AVS)
    bool subpixel = parameters().get_bool("bilinear", true);
    bool wrap = parameters().get_bool("wrap", false);
    bool blend = parameters().get_bool("blend", false);
    grid_.apply(framebuffer, fbout, w, h, subpixel, wrap, blend);

    return 1; // Use fbout
}

void DynamicMovementEffect::execute_init_script(AudioData visdata, int w, int h) {
    // TODO: Implement actual EEL script execution
    // For now, this is a stub to resolve linker error
}

void DynamicMovementEffect::execute_frame_script(AudioData visdata, int w, int h) {
    // TODO: Implement actual EEL script execution
    // For now, this is a stub to resolve linker error
}

void DynamicMovementEffect::execute_beat_script(AudioData visdata, int w, int h) {
    // TODO: Implement actual EEL script execution
    // For now, this is a stub to resolve linker error
}

// Static member definition
const PluginInfo DynamicMovementEffect::effect_info {
    .name = "Dynamic Movement",
    .category = "Trans",
    .description = "Grid-based transformations with scripting",
    .author = "",
    .version = 1,
    .legacy_index = 43,  // R_DMove
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<DynamicMovementEffect>();
    },
    .ui_layout = {
        {
            // Script editors - original positions from r_dmove.cpp dialog
            {
                .id = "init_script",
                .text = "Init",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 0, .w = 208, .h = 14
            },
            {
                .id = "frame_script",
                .text = "Frame",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 14, .w = 208, .h = 53
            },
            {
                .id = "beat_script",
                .text = "Beat",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 67, .w = 208, .h = 53
            },
            {
                .id = "pixel_script",
                .text = "Pixel",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 120, .w = 208, .h = 53
            },
            // Grid size
            {
                .id = "grid_width",
                .text = "Grid W",
                .type = ControlType::TEXT_INPUT,
                .x = 108, .y = 190, .w = 24, .h = 12,
                .range = {2, 256},
                .default_val = 16
            },
            {
                .id = "grid_height",
                .text = "Grid H",
                .type = ControlType::TEXT_INPUT,
                .x = 160, .y = 190, .w = 24, .h = 12,
                .range = {2, 256},
                .default_val = 16
            },
            // Checkboxes
            {
                .id = "blend",
                .text = "Blend",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 190, .w = 34, .h = 10,
                .default_val = 0
            },
            {
                .id = "wrap",
                .text = "Wrap",
                .type = ControlType::CHECKBOX,
                .x = 35, .y = 190, .w = 33, .h = 10,
                .default_val = 0
            },
            {
                .id = "no_movement",
                .text = "No movement",
                .type = ControlType::CHECKBOX,
                .x = 113, .y = 176, .w = 100, .h = 10,
                .default_val = 0
            },
            {
                .id = "rectangular",
                .text = "Rectangular coords",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 204, .w = 90, .h = 10,
                .default_val = 0
            },
            {
                .id = "bilinear",
                .text = "Bilinear",
                .type = ControlType::CHECKBOX,
                .x = 94, .y = 204, .w = 63, .h = 10,
                .default_val = 1
            }
        }
    }
};

// Register effect at startup
static bool register_dynamic_movement = []() {
    PluginManager::instance().register_effect(DynamicMovementEffect::effect_info);
    return true;
}();

} // namespace avs