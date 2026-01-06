// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once
#include <vector>
#include <string>

namespace avs {

enum class ControlType {
    CHECKBOX,
    SLIDER,
    BUTTON,
    RADIO_GROUP,     // Group of mutually exclusive radio buttons
    TEXT_INPUT,      // Single-line text input
    EDITTEXT,        // Multi-line text edit (for scripts)
    COLOR_BUTTON,
    COLOR_ARRAY,     // Multi-color bar with clickable segments (uses color_0..color_N params)
    DROPDOWN,
    LABEL,           // Static text label (LTEXT in Windows)
    GROUPBOX         // Visual grouping box with title
};

// Common enums for radio group values
enum class BlendMode { REPLACE = 0, ADDITIVE = 1, BLEND_5050 = 2, DEFAULT = 3 };
enum class DrawStyle { LINES = 0, SOLID = 1, DOTS = 2 };
enum class AudioChannel { LEFT = 0, RIGHT = 1, CENTER = 2 };
enum class VerticalPosition { TOP = 0, BOTTOM = 1, CENTER = 2 };
enum class RenderMode { SPECTRUM = 0, OSCILLOSCOPE = 1 };

struct RadioOption {
    std::string label;
    int x, y, w, h;
};

struct ControlRange {
    int min = 0;
    int max = 100;
    int tick_freq = 0;  // For slider tick marks
};

struct ControlLayout {
    std::string id;              // Parameter name this control maps to
    std::string text;            // Display text / label
    ControlType type;
    int x, y, w, h;              // Position and size from original dialog
    ControlRange range = {0, 100};  // For sliders: min, max, tick_freq
    int default_val = 0;            // Default value for all control types
    std::vector<std::string> options; // For dropdowns
    std::vector<RadioOption> radio_options; // For RADIO_GROUP: each option with position
    int max_items = 16;             // For array controls (e.g., COLOR_ARRAY): max item count
    bool enabled = true;
};

/**
 * Data-driven effect UI layout
 * Contains original AVS dialog layout information for any UI system to use
 *
 * NOTE: All original AVS effect dialogs are 137x137 pixels
 */
class EffectUILayout {
public:
    std::vector<ControlLayout> controls;

    // Constructor for easy initialization
    EffectUILayout(const std::vector<ControlLayout>& ctrls) : controls(ctrls) {}

    // Default constructor
    EffectUILayout() = default;

    // Accessor methods
    const std::vector<ControlLayout>& getControls() const { return controls; }

    // Helper methods for common operations
    ControlLayout getControl(const std::string& id) const {
        for (const auto& control : controls) {
            if (control.id == id) return control;
        }
        return {}; // Return empty if not found
    }

    bool hasControl(const std::string& id) const {
        for (const auto& control : controls) {
            if (control.id == id) return true;
        }
        return false;
    }
};

} // namespace avs