#pragma once
#include "ui_layout.h"
#include <memory>
#include <unordered_map>

namespace avs {

class UILayoutRegistry {
public:
    static UILayoutRegistry& instance() {
        static UILayoutRegistry registry;
        return registry;
    }
    
    void registerLayout(const std::string& effect_name, std::unique_ptr<EffectUILayout> layout) {
        layouts_[effect_name] = std::move(layout);
    }
    
    const EffectUILayout* getLayout(const std::string& effect_name) const {
        auto it = layouts_.find(effect_name);
        return (it != layouts_.end()) ? it->second.get() : nullptr;
    }
    
    std::vector<std::string> getAvailableLayouts() const {
        std::vector<std::string> names;
        for (const auto& pair : layouts_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
private:
    std::unordered_map<std::string, std::unique_ptr<EffectUILayout>> layouts_;
};

// Helper macro for registering layouts
#define REGISTER_UI_LAYOUT(effect_name, layout_class) \
    static bool registered_##layout_class = []() { \
        UILayoutRegistry::instance().registerLayout(effect_name, \
            std::make_unique<layout_class>()); \
        return true; \
    }();

} // namespace avs