// avs_lib - Portable Advanced Visualization Studio library
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "preset.h"
#include "json.h"
#include "plugin_manager.h"
#include "effect_container.h"
#include <fstream>
#include <sstream>

namespace avs {

std::string Preset::last_error_;

// ============================================================================
// Helper functions for parameter serialization
// ============================================================================

static JsonValue param_to_json(const Parameter& param) {
    switch (param.type()) {
        case ParameterType::FLOAT:
            return param.as_float();
        case ParameterType::INT:
        case ParameterType::ENUM:
            return param.as_int();
        case ParameterType::BOOL:
            return param.as_bool();
        case ParameterType::COLOR:
            // Store as double to preserve full uint32_t range (int would make 0xFFFFFFFF into -1)
            return static_cast<double>(param.as_color());
        case ParameterType::STRING:
            return param.as_string();
        default:
            return nullptr;
    }
}

static void json_to_param(const JsonValue& json, Parameter& param) {
    switch (param.type()) {
        case ParameterType::FLOAT:
            if (json.is_number()) param.set_value(json.as_number());
            break;
        case ParameterType::INT:
        case ParameterType::ENUM:
            if (json.is_number()) param.set_value(json.as_int());
            break;
        case ParameterType::BOOL:
            if (json.is_bool()) param.set_value(json.as_bool());
            break;
        case ParameterType::COLOR:
            if (json.is_number()) param.set_value(static_cast<uint32_t>(json.as_int()));
            break;
        case ParameterType::STRING:
            if (json.is_string()) param.set_value(json.as_string());
            break;
    }
}

// ============================================================================
// Effect serialization
// ============================================================================

static JsonValue effect_to_json(const EffectBase* effect) {
    JsonObject obj;

    // Effect type name
    obj["type"] = effect->get_plugin_info().name;

    // Enabled state
    obj["enabled"] = effect->is_enabled();

    // Parameters
    JsonObject params;

    // Check for num_colors to limit color array serialization
    int num_colors = effect->parameters().get_int("num_colors", 0);

    for (const auto& [name, param] : effect->parameters().all_parameters()) {
        // Skip 'enabled' - we handle it separately
        if (name == "enabled") continue;

        // Skip color_N parameters beyond num_colors
        if (num_colors > 0 && name.substr(0, 6) == "color_") {
            int color_idx = std::stoi(name.substr(6));
            if (color_idx >= num_colors) continue;
        }

        params[name] = param_to_json(*param);
    }
    if (!params.empty()) {
        obj["params"] = std::move(params);
    }

    // Children (if container)
    const auto* container = dynamic_cast<const EffectContainer*>(effect);
    if (container && container->child_count() > 0) {
        JsonArray children;
        for (size_t i = 0; i < container->child_count(); i++) {
            children.push_back(effect_to_json(container->get_child(i)));
        }
        obj["effects"] = std::move(children);
    }

    return obj;
}

static std::unique_ptr<EffectBase> json_to_effect(const JsonValue& json) {
    if (!json.is_object()) return nullptr;

    // Get effect type
    if (!json.has("type") || !json["type"].is_string()) return nullptr;
    std::string type_name = json["type"].as_string();

    // Create effect instance
    auto effect = PluginManager::instance().create_effect(type_name);
    if (!effect) return nullptr;

    // Set enabled state
    if (json.has("enabled") && json["enabled"].is_bool()) {
        effect->set_enabled(json["enabled"].as_bool());
    }

    // Set parameters
    if (json.has("params") && json["params"].is_object()) {
        const auto& params_json = json["params"].as_object();
        for (const auto& [name, value] : params_json) {
            auto param = effect->parameters().get_parameter(name);
            if (param) {
                json_to_param(value, *param);
            }
        }
    }

    // Load children (if container)
    auto* container = dynamic_cast<EffectContainer*>(effect.get());
    if (container && json.has("effects") && json["effects"].is_array()) {
        for (const auto& child_json : json["effects"].as_array()) {
            auto child = json_to_effect(child_json);
            if (child) {
                container->add_child(std::move(child));
            }
        }
    }

    return effect;
}

// ============================================================================
// Public API
// ============================================================================

std::string Preset::to_json(const EffectContainer& root) {
    JsonObject preset;
    preset["version"] = "1.0";
    preset["format"] = "avs-json";

    // Serialize all children of root
    JsonArray effects;
    for (size_t i = 0; i < root.child_count(); i++) {
        effects.push_back(effect_to_json(root.get_child(i)));
    }
    preset["effects"] = std::move(effects);

    return json_write(preset, true);
}

bool Preset::from_json(const std::string& json, EffectContainer& root) {
    try {
        JsonValue parsed = json_parse(json);

        if (!parsed.is_object()) {
            last_error_ = "Invalid JSON: expected object at root";
            return false;
        }

        // Clear existing effects
        while (root.child_count() > 0) {
            root.remove_child(0);
        }

        // Load effects array
        if (parsed.has("effects") && parsed["effects"].is_array()) {
            for (const auto& effect_json : parsed["effects"].as_array()) {
                auto effect = json_to_effect(effect_json);
                if (effect) {
                    root.add_child(std::move(effect));
                }
            }
        }

        last_error_.clear();
        return true;

    } catch (const std::exception& e) {
        last_error_ = std::string("JSON parse error: ") + e.what();
        return false;
    }
}

bool Preset::save_json(const std::string& path, const EffectContainer& root) {
    try {
        std::string json = to_json(root);

        std::ofstream file(path);
        if (!file.is_open()) {
            last_error_ = "Failed to open file for writing: " + path;
            return false;
        }

        file << json;
        file.close();

        if (file.fail()) {
            last_error_ = "Failed to write to file: " + path;
            return false;
        }

        last_error_.clear();
        return true;

    } catch (const std::exception& e) {
        last_error_ = std::string("Save error: ") + e.what();
        return false;
    }
}

bool Preset::load_json(const std::string& path, EffectContainer& root) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            last_error_ = "Failed to open file: " + path;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json = buffer.str();

        return from_json(json, root);

    } catch (const std::exception& e) {
        last_error_ = std::string("Load error: ") + e.what();
        return false;
    }
}

PresetFormat Preset::detect_format(const std::string& path) {
    // Check extension
    size_t dot = path.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = path.substr(dot);
        if (ext == ".json") return PresetFormat::JSON;
        if (ext == ".avs") return PresetFormat::LEGACY;
    }

    // Try to detect from content
    std::ifstream file(path, std::ios::binary);
    if (file.is_open()) {
        char header[25];
        file.read(header, 25);
        if (file.gcount() >= 25) {
            // Check for AVS legacy header
            if (std::string(header, 23) == "Nullsoft AVS Preset 0.") {
                return PresetFormat::LEGACY;
            }
        }
    }

    // Default to JSON
    return PresetFormat::JSON;
}

bool Preset::save(const std::string& path, const EffectContainer& root, PresetFormat format) {
    if (format == PresetFormat::AUTO) {
        format = detect_format(path);
    }

    switch (format) {
        case PresetFormat::JSON:
            return save_json(path, root);
        case PresetFormat::LEGACY:
            last_error_ = "Legacy AVS format saving not yet implemented";
            return false;
        default:
            return save_json(path, root);
    }
}

bool Preset::load(const std::string& path, EffectContainer& root, PresetFormat format) {
    if (format == PresetFormat::AUTO) {
        format = detect_format(path);
    }

    switch (format) {
        case PresetFormat::JSON:
            return load_json(path, root);
        case PresetFormat::LEGACY:
            last_error_ = "Legacy AVS format loading not yet implemented";
            return false;
        default:
            return load_json(path, root);
    }
}

} // namespace avs
