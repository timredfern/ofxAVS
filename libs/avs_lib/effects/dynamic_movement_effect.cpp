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
    : script_initialized_(false)
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

    regenerate_grid();
}

void DynamicMovementEffect::regenerate_grid() {
    int grid_width = parameters().get_int("grid_width", 16);
    int grid_height = parameters().get_int("grid_height", 16);
    bool rectangular = parameters().get_bool("rectangular", false);
    std::string pixel_script = parameters().get_string("pixel_script");

    // Generate grid using normalized coordinates
    // Use 256x256 as reference for script sw/sh variables (aspect ratio 1:1)
    // The actual output dimensions are handled by apply()
    static char dummy_audio[2][2][576] = {{{0}}};
    CoordMode mode = rectangular ? CoordMode::RECTANGULAR : CoordMode::POLAR;
    grid_.generate(grid_width, grid_height, 256, 256, pixel_script, mode, dummy_audio);
}

void DynamicMovementEffect::on_parameter_changed(const std::string& param_name) {
    // Regenerate grid when grid-affecting parameters change
    if (param_name == "grid_width" || param_name == "grid_height" ||
        param_name == "pixel_script" || param_name == "rectangular") {
        regenerate_grid();
    }
    // Re-run init script when it changes
    if (param_name == "init_script") {
        script_initialized_ = false;
    }
}

int DynamicMovementEffect::render(AudioData visdata, int isBeat,
                                 uint32_t* framebuffer, uint32_t* fbout,
                                 int w, int h) {
    (void)visdata; (void)isBeat;  // Grid is pre-generated, these are for future frame/beat scripts

    if (!is_enabled()) return 0;

    if (parameters().get_bool("no_movement", false)) {
        memcpy(fbout, framebuffer, w * h * sizeof(uint32_t));
        return 1;
    }

    // Apply pre-generated grid with runtime dimensions
    bool subpixel = parameters().get_bool("bilinear", true);
    bool wrap = parameters().get_bool("wrap", false);
    bool blend = parameters().get_bool("blend", false);
    grid_.apply(framebuffer, fbout, w, h, subpixel, wrap, blend);

    return 1;
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
            // Labels for script boxes (from res.rc IDD_CFG_DMOVE)
            {
                .id = "init_label",
                .text = "init",
                .type = ControlType::LABEL,
                .x = 0, .y = 3, .w = 10, .h = 8
            },
            {
                .id = "frame_label",
                .text = "frame",
                .type = ControlType::LABEL,
                .x = 0, .y = 36, .w = 18, .h = 8
            },
            {
                .id = "beat_label",
                .text = "beat",
                .type = ControlType::LABEL,
                .x = 0, .y = 89, .w = 15, .h = 8
            },
            {
                .id = "pixel_label",
                .text = "pixel",
                .type = ControlType::LABEL,
                .x = 0, .y = 142, .w = 15, .h = 8
            },
            // Script editors - IDC_EDIT4, IDC_EDIT2, IDC_EDIT3, IDC_EDIT1
            {
                .id = "init_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 0, .w = 208, .h = 14
            },
            {
                .id = "frame_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 14, .w = 208, .h = 53
            },
            {
                .id = "beat_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 67, .w = 208, .h = 53
            },
            {
                .id = "pixel_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 120, .w = 208, .h = 53
            },
            // Grid size labels
            {
                .id = "gridsize_label",
                .text = "Grid size:",
                .type = ControlType::LABEL,
                .x = 78, .y = 192, .w = 30, .h = 8
            },
            {
                .id = "gridx_label",
                .text = "x",
                .type = ControlType::LABEL,
                .x = 128, .y = 191, .w = 8, .h = 8
            },
            // Grid size inputs - IDC_EDIT5, IDC_EDIT6
            {
                .id = "grid_width",
                .text = "",
                .type = ControlType::TEXT_INPUT,
                .x = 108, .y = 190, .w = 18, .h = 12,
                .range = {2, 256},
                .default_val = 16
            },
            {
                .id = "grid_height",
                .text = "",
                .type = ControlType::TEXT_INPUT,
                .x = 136, .y = 190, .w = 18, .h = 12,
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