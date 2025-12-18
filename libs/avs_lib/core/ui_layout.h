#pragma once
#include <vector>
#include <string>

// Forward declare to avoid including ImGui in header
struct ImGuiContext;

namespace avs {

enum class ControlType {
    CHECKBOX,
    SLIDER,
    BUTTON,
    RADIO_BUTTON,
    TEXT_INPUT,
    COLOR_BUTTON,
    DROPDOWN
};

struct ControlRange {
    int min;
    int max;
    int default_val;
    int tick_freq = 0;  // For slider tick marks
};

struct ControlLayout {
    std::string id;              // Parameter name this control maps to
    std::string text;            // Display text / label
    ControlType type;
    int x, y, w, h;              // Position and size from original dialog
    ControlRange range = {0, 100, 50}; // For sliders/numeric controls
    std::vector<std::string> options; // For dropdowns/radio groups
    bool enabled = true;
};

/**
 * Base class for effect UI layout metadata
 * Provides original AVS dialog layout information for any UI system to use
 * 
 * NOTE: All original AVS effect dialogs are 137x137 pixels
 */
class EffectUILayout {
public:
    virtual ~EffectUILayout() = default;
    
    virtual std::vector<ControlLayout> getControls() const = 0;
    virtual std::string getEffectName() const = 0;
    
    // Helper methods for common operations
    ControlLayout getControl(const std::string& id) const {
        for (const auto& control : getControls()) {
            if (control.id == id) return control;
        }
        return {}; // Return empty if not found
    }
    
    bool hasControl(const std::string& id) const {
        for (const auto& control : getControls()) {
            if (control.id == id) return true;
        }
        return false;
    }
    
    // ImGui rendering method - implemented in .cpp file
    virtual void renderImGui(class EffectBase* effect) const;
};

} // namespace avs