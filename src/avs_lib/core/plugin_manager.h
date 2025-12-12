#pragma once

#include "effect_base.h"
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <vector>

namespace avs {

// Plugin factory function type
using EffectFactory = std::function<std::unique_ptr<EffectBase>()>;

// Plugin registration info
struct PluginInfo {
    std::string name;
    std::string description;
    std::string author;
    int version;
    EffectFactory factory;
};

class PluginManager {
public:
    static PluginManager& instance() {
        static PluginManager mgr;
        return mgr;
    }
    
    // Register built-in effects
    void register_effect(const std::string& id, const PluginInfo& info);
    
    // Create effect instances
    std::unique_ptr<EffectBase> create_effect(const std::string& id);
    
    // Query available effects
    std::vector<std::string> available_effects() const;
    PluginInfo get_effect_info(const std::string& id) const;
    
    // Future: dynamic plugin loading
    // bool load_plugin_library(const std::string& path);
    // void unload_all_plugins();

private:
    std::map<std::string, PluginInfo> registered_effects_;
    
    PluginManager() = default;
    ~PluginManager() = default;
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
};

// Macro to make effect registration easier
#define REGISTER_AVS_EFFECT(id, effect_class) \
    static struct effect_class##_registrar { \
        effect_class##_registrar() { \
            avs::PluginInfo info; \
            info.name = #effect_class; \
            info.description = ""; \
            info.author = ""; \
            info.version = 1; \
            info.factory = []() -> std::unique_ptr<avs::EffectBase> { \
                return std::make_unique<effect_class>(); \
            }; \
            avs::PluginManager::instance().register_effect(id, info); \
        } \
    } effect_class##_reg;

} // namespace avs