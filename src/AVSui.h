// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/ui.h"
#include "core/effect_base.h"

namespace avs_ui {

/**
 * Render an effect's UI layout using ImGui
 * This function provides the ImGui-specific rendering for avs::EffectUILayout
 *
 * @param layout The UI layout describing the controls
 * @param effect The effect instance to read/write parameters from
 */
void renderImGui(const avs::EffectUILayout& layout, avs::EffectBase* effect);

} // namespace avs_ui
