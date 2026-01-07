// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

#pragma once

#include "parameter.h"
#include "ui.h"
#include <string>

namespace avs {

// Interface for anything that can be configured via the parameters panel.
// This allows effects, beat detector, and future settings to share the same UI system.
class Configurable {
public:
    virtual ~Configurable() = default;

    // Display name shown in the tree view and parameters panel header
    virtual std::string get_display_name() const = 0;

    // UI layout defining the controls to show in the parameters panel
    virtual const EffectUILayout& get_ui_layout() const = 0;

    // Parameter storage for reading/writing values
    virtual ParameterGroup& parameters() = 0;
    virtual const ParameterGroup& parameters() const = 0;

    // Called when a parameter is changed via UI
    // Override to respond to parameter changes (e.g., sync dependent parameters)
    virtual void on_parameter_changed(const std::string& /*param_name*/) {}
};

} // namespace avs
