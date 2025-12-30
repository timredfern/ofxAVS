// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/ui.h"
#include "core/configurable.h"

namespace avs_ui {

/**
 * Render a Configurable's UI layout using ImGui
 * This function provides the ImGui-specific rendering for avs::EffectUILayout
 * Works with any Configurable: effects, beat detector, and other settings
 *
 * @param layout The UI layout describing the controls
 * @param configurable The Configurable instance to read/write parameters from
 */
void renderImGui(const avs::EffectUILayout& layout, avs::Configurable* configurable);

} // namespace avs_ui
