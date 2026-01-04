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

namespace avs {

void register_builtin_effects() {
    // Effects are now self-registering through static initialization in each effect file
    // This function is kept for backward compatibility but doesn't need to do anything
    // The effects register themselves with their complete UI layouts
}

} // namespace avs