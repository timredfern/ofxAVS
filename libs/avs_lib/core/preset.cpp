// avs_lib - Portable Advanced Visualization Studio library
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "preset.h"
#include "binary_reader.h"
#include "json.h"
#include "plugin_manager.h"
#include "effect_container.h"
#include "effects/unsupported_effect.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>

namespace avs {

// ============================================================================
// Binary format constants and helpers
// ============================================================================

static const char AVS_HEADER[] = "Nullsoft AVS Preset 0.";
static const size_t AVS_HEADER_LEN = 25;  // 22 (prefix) + 1 (version) + 1 (0x1a) + 1 (root mode)
static const uint32_t EFFECT_LIST_INDEX = 0xFFFFFFFE;
static const uint32_t DLLRENDERBASE = 16384;

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
        case ParameterType::COLOR: {
            // Store as hex string for readability (e.g., "#FF0000FF")
            char hex[10];
            snprintf(hex, sizeof(hex), "#%08X", param.as_color());
            return std::string(hex);
        }
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
            if (json.is_string()) {
                // Parse hex string like "#FF0000FF"
                std::string s = json.as_string();
                if (!s.empty() && s[0] == '#') {
                    param.set_value(static_cast<uint32_t>(std::stoul(s.substr(1), nullptr, 16)));
                }
            }
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
    int num_colors = effect->parameters().has_parameter("num_colors")
                     ? effect->parameters().get_int("num_colors") : 0;

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

    // Save root settings
    preset["clear_each_frame"] = root.parameters().get_bool("clear_each_frame");

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

        // Load root settings
        if (parsed.has("clear_each_frame") && parsed["clear_each_frame"].is_bool()) {
            root.parameters().set_bool("clear_each_frame", parsed["clear_each_frame"].as_bool());
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

// ============================================================================
// Legacy binary format loading
// ============================================================================

// Forward declaration for recursive Effect List loading
static bool load_effect_list_children(BinaryReader& reader, EffectContainer* container);

static std::unique_ptr<EffectBase> load_legacy_effect(BinaryReader& reader) {
    if (reader.remaining() < 8) return nullptr;

    uint32_t effect_index = reader.read_u32();
    std::string plugin_id;

    // Check for plugin effect (has string ID)
    if (effect_index >= DLLRENDERBASE && effect_index != EFFECT_LIST_INDEX) {
        plugin_id = reader.read_string_fixed(32);
    }

    uint32_t config_length = reader.read_u32();

    // Create effect instance
    std::unique_ptr<EffectBase> effect;

    if (effect_index == EFFECT_LIST_INDEX) {
        // Effect List - special handling
        effect = PluginManager::instance().create_effect("Effect List");

        if (effect && config_length > 0) {
            // Read mode byte
            uint8_t mode = reader.read_u8();
            size_t consumed = 1;

            // Extended data if high bit set
            if (mode & 0x80) {
                reader.skip(4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4); // mode, blend vals, buffers, etc
                consumed += 36;
            }

            // For non-root Effect Lists, there's additional config
            // Check for "AVS 2.8+ Effect List Config" marker
            if (config_length > consumed + 36) {
                size_t remaining_config = config_length - consumed;
                // Just skip the Effect List's own config for now
                // Child effects follow after config_length bytes total
            }

            // Skip rest of Effect List config
            if (config_length > consumed) {
                reader.skip(config_length - consumed);
            }

            // Load children recursively
            auto* container = dynamic_cast<EffectContainer*>(effect.get());
            if (container) {
                load_effect_list_children(reader, container);
            }
        }
    } else if (effect_index < DLLRENDERBASE) {
        // Built-in effect by index
        effect = PluginManager::instance().create_by_legacy_index(static_cast<int>(effect_index));

        // If effect not implemented, create placeholder with the effect name
        if (!effect) {
            const char* name = get_legacy_effect_name(static_cast<int>(effect_index));
            if (name) {
                effect = std::make_unique<UnsupportedEffect>(name, static_cast<int>(effect_index));
            } else {
                // Unknown effect index
                effect = std::make_unique<UnsupportedEffect>(
                    "Unknown Effect #" + std::to_string(effect_index),
                    static_cast<int>(effect_index));
            }
        }

        // Pass config data to effect for parsing
        if (effect && config_length > 0 && reader.remaining() >= config_length) {
            std::vector<uint8_t> config_data(reader.ptr(), reader.ptr() + config_length);
            effect->load_parameters(config_data);
        }
        reader.skip(config_length);
    } else {
        // Plugin effect by string ID
        effect = std::make_unique<UnsupportedEffect>(
            "Plugin: " + plugin_id,
            static_cast<int>(effect_index));
        reader.skip(config_length);
    }

    return effect;
}

static bool load_effect_list_children(BinaryReader& reader, EffectContainer* container) {
    // Load effects until we hit end of data or another Effect List end marker
    while (!reader.eof()) {
        // Peek at next effect index
        if (reader.remaining() < 4) break;

        auto effect = load_legacy_effect(reader);
        if (effect) {
            container->add_child(std::move(effect));
        } else {
            // Unknown effect or parse error - stop
            break;
        }
    }
    return true;
}

bool Preset::load_legacy(const std::string& path, EffectContainer& root) {
    try {
        // Read entire file
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            last_error_ = "Failed to open file: " + path;
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(size);
        if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
            last_error_ = "Failed to read file: " + path;
            return false;
        }

        return from_legacy(data, root);

    } catch (const std::exception& e) {
        last_error_ = std::string("Load error: ") + e.what();
        return false;
    }
}

bool Preset::from_legacy(const std::vector<uint8_t>& data, EffectContainer& root) {
    if (data.size() < AVS_HEADER_LEN) {
        last_error_ = "File too small for AVS header";
        return false;
    }

    BinaryReader reader(data);

    // Validate header
    std::string header = reader.read_string_fixed(22);
    if (header != AVS_HEADER) {
        last_error_ = "Invalid AVS header";
        return false;
    }

    // Version byte (position 22)
    uint8_t version = reader.read_u8();
    if (version != '1' && version != '2') {
        last_error_ = "Unsupported AVS version";
        return false;
    }

    // Skip EOF marker (0x1a at position 23)
    reader.skip(1);

    // Root mode byte (position 24)
    // Bit 0 = clear every frame
    uint8_t mode = reader.read_u8();
    root.parameters().set_bool("clear_each_frame", (mode & 1) != 0);

    // Clear existing effects
    while (root.child_count() > 0) {
        root.remove_child(0);
    }

    // Load effects
    int loaded = 0;
    int skipped = 0;

    while (!reader.eof() && reader.remaining() >= 8) {
        auto effect = load_legacy_effect(reader);
        if (effect) {
            root.add_child(std::move(effect));
            loaded++;
        } else {
            skipped++;
        }
    }

    if (loaded == 0 && skipped > 0) {
        last_error_ = "No supported effects found in preset";
        return false;
    }

    last_error_.clear();
    return true;
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
            return load_legacy(path, root);
        default:
            return load_json(path, root);
    }
}

} // namespace avs
