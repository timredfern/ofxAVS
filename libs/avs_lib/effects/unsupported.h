// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/plugin_manager.h"
#include <string>

namespace avs {

// Placeholder effect for unsupported/unimplemented effects
// Displays the effect name so users know what's missing
class UnsupportedEffect : public EffectBase {
public:
    UnsupportedEffect(const std::string& effect_name, int legacy_index = -1)
        : effect_name_(effect_name), legacy_index_(legacy_index) {
        // Create a custom plugin info for this instance
        info_.name = "Unsupported: " + effect_name;
        info_.category = "Misc";
        info_.description = "Effect not yet implemented";
        info_.legacy_index = legacy_index;
    }

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override {
        // Do nothing - just a placeholder
        return 0;
    }

    const PluginInfo& get_plugin_info() const override { return info_; }

    const std::string& original_name() const { return effect_name_; }
    int original_legacy_index() const { return legacy_index_; }

private:
    std::string effect_name_;
    int legacy_index_;
    PluginInfo info_;
};

// Get the name of an effect by its legacy index
// Returns empty string if index is unknown
const char* get_legacy_effect_name(int legacy_index);

} // namespace avs
