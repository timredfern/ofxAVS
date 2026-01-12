// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

#include "effect_base.h"
#include "plugin_manager.h"

namespace avs {

std::string EffectBase::get_display_name() const {
    return get_plugin_info().name;
}

const EffectUILayout& EffectBase::get_ui_layout() const {
    return get_plugin_info().ui_layout;
}

std::string EffectBase::get_help_text() const {
    return get_plugin_info().help_text;
}

} // namespace avs
