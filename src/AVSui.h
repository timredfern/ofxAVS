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
 * If configurable->get_help_text() is non-empty, an "Expression Help" button is rendered.
 *
 * @param layout The UI layout describing the controls
 * @param configurable The Configurable instance to read/write parameters from
 */
void renderImGui(const avs::EffectUILayout& layout, avs::Configurable* configurable);

/**
 * Render an "Expression Help" button and manage the help popup
 * Call this after renderImGui() for scripted effects that have help text.
 * Only one help popup can be open at a time (global state).
 * Position should be set with ImGui::SetCursorPos() before calling.
 *
 * @param effect_name Name to display on the effect-specific tab
 * @param effect_help Effect-specific help text (variables, etc.)
 * @param width Button width (default 142 = 71*2 matching original AVS scale)
 * @param height Button height (default 24 = 12*2 matching original AVS scale)
 */
void renderExpressionHelpButton(const std::string& effect_name, const std::string& effect_help,
                                 float width = 142.0f, float height = 24.0f);

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
