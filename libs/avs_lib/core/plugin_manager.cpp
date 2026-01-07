// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "plugin_manager.h"

namespace avs {

void PluginManager::register_effect(const PluginInfo& info) {
    registered_effects_[info.name] = info;

    // Also register legacy index mapping if provided
    if (info.legacy_index != LEGACY_INDEX_NONE) {
        legacy_index_map_[info.legacy_index] = info.name;
    }
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

const EffectUILayout* PluginManager::get_ui_layout(const std::string& id) const {
    auto it = registered_effects_.find(id);
    return (it != registered_effects_.end()) ? &it->second.ui_layout : nullptr;
}

const PluginInfo* PluginManager::get_by_legacy_index(int legacy_index) const {
    auto idx_it = legacy_index_map_.find(legacy_index);
    if (idx_it == legacy_index_map_.end()) {
        return nullptr;
    }
    auto it = registered_effects_.find(idx_it->second);
    return (it != registered_effects_.end()) ? &it->second : nullptr;
}

std::unique_ptr<EffectBase> PluginManager::create_by_legacy_index(int legacy_index) {
    const PluginInfo* info = get_by_legacy_index(legacy_index);
    if (info && info->factory) {
        return info->factory();
    }
    return nullptr;
}

} // namespace avs