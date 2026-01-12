// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "builtin_effects.h"
#include "plugin_manager.h"
#include "effects/clear.h"
#include "effects/oscilloscope.h"
#include "effects/superscope.h"
#include "effects/blur.h"
#include "effects/brightness.h"
#include "effects/movement.h"
#include "effects/dynamic_movement.h"
#include "effects/onbeat_clear.h"
#include "effects/dot_grid.h"
#include "effects/color_fade.h"
#include "effects/ddm.h"
#include "effects/fadeout.h"
#include "effects/bump.h"
#include "effects/effect_list.h"
#include "effects/rotoblitter.h"
#include "effects/custom_bpm.h"
#include "effects/set_render_mode.h"
#include "effects/mirror.h"
#include "effects/shift.h"
#include "effects/scatter.h"
#include "effects/dot_fountain.h"
#include "effects/water.h"
#include "effects/moving_particle.h"
#include "effects/interferences.h"

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
    pm.register_effect(ColorFadeEffect::effect_info);
    pm.register_effect(DDMEffect::effect_info);
    pm.register_effect(FadeoutEffect::effect_info);
    pm.register_effect(BumpEffect::effect_info);
    pm.register_effect(EffectList::effect_info);
    pm.register_effect(RotoBlitterEffect::effect_info);
    pm.register_effect(CustomBpmEffect::effect_info);
    pm.register_effect(SetRenderModeEffect::effect_info);
    pm.register_effect(MirrorEffect::effect_info);
    pm.register_effect(ShiftEffect::effect_info);
    pm.register_effect(ScatterEffect::effect_info);
    pm.register_effect(DotFountainEffect::effect_info);
    pm.register_effect(WaterEffect::effect_info);
    pm.register_effect(MovingParticleEffect::effect_info);
    pm.register_effect(InterferencesEffect::effect_info);
}

} // namespace avs