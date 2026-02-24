// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/ui.h"
#include "core/configurable.h"
#include <string>

namespace avs_ui {

/**
 * Render a Configurable's UI layout using ImGui
 * This function provides the ImGui-specific rendering for avs::EffectUILayout
 * Works with any Configurable: effects, beat detector, and other settings
 * Script edit boxes have right-click context menu with "Expression Help" option.
 *
 * @param layout The UI layout describing the controls
 * @param configurable The Configurable instance to read/write parameters from
 */
void renderImGui(const avs::EffectUILayout& layout, avs::Configurable* configurable);

/**
 * Render the expression help popup window
 * Call this once per frame (e.g., in your main UI loop) to draw the popup if open.
 * The popup is a floating ImGui window with tabs for General, Operators, Functions,
 * Constants, and the effect-specific help.
 */
void renderExpressionHelpPopup();

/**
 * Render a Configurable's parameters in a fullscreen borderless ImGui window
 * Use this for separate OS windows where the window title is already set.
 * Creates a borderless ImGui window filling the current OpenFrameworks window.
 *
 * @param configurable The Configurable instance to render parameters for
 * @param window_width Current window width (typically ofGetWidth())
 * @param window_height Current window height (typically ofGetHeight())
 */
void renderParamWindowContent(avs::Configurable* configurable, float window_width, float window_height);

} // namespace avs_ui
