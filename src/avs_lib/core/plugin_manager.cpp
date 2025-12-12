#include "plugin_manager.h"

namespace avs {

void PluginManager::register_effect(const std::string& id, const PluginInfo& info) {
    registered_effects_[id] = info;
}

std::unique_ptr<EffectBase> PluginManager::create_effect(const std::string& id) {
    auto it = registered_effects_.find(id);
    if (it != registered_effects_.end() && it->second.factory) {
        return it->second.factory();
    }
    return nullptr;
}

std::vector<std::string> PluginManager::available_effects() const {
    std::vector<std::string> effects;
    for (const auto& [id, info] : registered_effects_) {
        effects.push_back(id);
    }
    return effects;
}

PluginInfo PluginManager::get_effect_info(const std::string& id) const {
    auto it = registered_effects_.find(id);
    return (it != registered_effects_.end()) ? it->second : PluginInfo{};
}

} // namespace avs