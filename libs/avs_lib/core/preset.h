// avs_lib - Portable Advanced Visualization Studio library
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "effect_container.h"
#include <string>
#include <vector>
#include <cstdint>

namespace avs {

// Preset file format types
enum class PresetFormat {
    AUTO,       // Detect from file extension/content
    JSON,       // Modern JSON format
    LEGACY      // Original AVS binary format
};

// Preset loader/saver
class Preset {
public:
    // Save effect chain to file
    // Returns true on success, false on error
    static bool save(const std::string& path, const EffectContainer& root,
                     PresetFormat format = PresetFormat::JSON);

    // Load effect chain from file
    // Returns true on success, false on error
    static bool load(const std::string& path, EffectContainer& root,
                     PresetFormat format = PresetFormat::AUTO);

    // Save/load to/from string (JSON only)
    static std::string to_json(const EffectContainer& root);
    static bool from_json(const std::string& json, EffectContainer& root);

    // Load from binary data (legacy AVS format)
    static bool from_legacy(const std::vector<uint8_t>& data, EffectContainer& root);

    // Detect format from file
    static PresetFormat detect_format(const std::string& path);

    // Get last error message
    static const std::string& last_error() { return last_error_; }

private:
    static bool save_json(const std::string& path, const EffectContainer& root);
    static bool load_json(const std::string& path, EffectContainer& root);
    static bool load_legacy(const std::string& path, EffectContainer& root);

    static std::string last_error_;
};

} // namespace avs
