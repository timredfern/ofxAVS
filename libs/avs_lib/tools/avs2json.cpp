// avs_lib - Portable Advanced Visualization Studio library
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License
//
// avs2json - Convert binary AVS presets to JSON format

#include "core/preset.h"
#include "core/builtin_effects.h"
#include "effects/effect_list.h"
#include <iostream>
#include <fstream>
#include <cstring>

void print_usage(const char* program) {
    std::cerr << "Usage: " << program << " <input.avs> [output.json]\n";
    std::cerr << "\nConverts binary AVS presets to JSON format.\n";
    std::cerr << "If output file is omitted, prints to stdout.\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Handle --help
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    const char* input_path = argv[1];
    const char* output_path = (argc > 2) ? argv[2] : nullptr;

    // Register built-in effects
    avs::register_builtin_effects();

    // Create root container
    avs::EffectList root;

    // Load binary preset
    if (!avs::Preset::load(input_path, root, avs::PresetFormat::LEGACY)) {
        std::cerr << "Error loading preset: " << avs::Preset::last_error() << "\n";
        return 1;
    }

    // Convert to JSON
    std::string json = avs::Preset::to_json(root);

    // Output
    if (output_path) {
        std::ofstream out(output_path);
        if (!out.is_open()) {
            std::cerr << "Error: Cannot open output file: " << output_path << "\n";
            return 1;
        }
        out << json;
        out.flush();
        if (!out.good()) {
            std::cerr << "Error: Failed to write to output file: " << output_path << "\n";
            return 1;
        }
        out.close();
        std::cerr << "Converted " << input_path << " -> " << output_path << " (" << json.size() << " bytes)\n";
    } else {
        std::cout << json;
    }

    return 0;
}
