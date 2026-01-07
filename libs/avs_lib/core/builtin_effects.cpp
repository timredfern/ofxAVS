// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "builtin_effects.h"
#include "plugin_manager.h"
#include "../effects/clear_effect.h"
#include "../effects/oscilloscope_effect.h"
#include "../effects/superscope_effect.h"
#include "../effects/blur_effect.h"
#include "../effects/brightness_effect.h"
#include "../effects/movement_effect.h"
#include "../effects/dynamic_movement_effect.h"
#include "../effects/onbeat_clear_effect.h"
#include "../effects/dot_grid_effect.h"
#include "../effects/effect_list.h"

namespace avs {

static bool effects_registered = false;

void register_builtin_effects() {
    if (effects_registered) return;
    effects_registered = true;

    auto& pm = PluginManager::instance();

    // Explicitly register all built-in effects
    // This is needed because static initialization in static libraries
    // may not trigger if the translation unit appears unused to the linker
    pm.register_effect(ClearEffect::effect_info);
    pm.register_effect(OscilloscopeEffect::effect_info);
    pm.register_effect(SuperScopeEffect::effect_info);
    pm.register_effect(BlurEffect::effect_info);
    pm.register_effect(BrightnessEffect::effect_info);
    pm.register_effect(MovementEffect::effect_info);
    pm.register_effect(DynamicMovementEffect::effect_info);
    pm.register_effect(OnBeatClearEffect::effect_info);
    pm.register_effect(DotGridEffect::effect_info);
    pm.register_effect(EffectList::effect_info);
}

} // namespace avs