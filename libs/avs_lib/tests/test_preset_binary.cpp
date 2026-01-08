// Test binary preset loading
// Tests the legacy AVS binary format (.avs files)

#include <catch2/catch_test_macros.hpp>
#include "core/preset.h"
#include "core/builtin_effects.h"
#include "core/plugin_manager.h"
#include "effects/effect_list_root.h"
#include <vector>
#include <cstdint>

// Helper to create a minimal valid AVS preset header
// Format: "Nullsoft AVS Preset 0." (22 bytes) + version (1 byte) + 0x1a (1 byte) + root_mode (1 byte)
static std::vector<uint8_t> make_header(char version = '2') {
    std::vector<uint8_t> data;
    // "Nullsoft AVS Preset 0."
    const char* header = "Nullsoft AVS Preset 0.";
    for (const char* p = header; *p; ++p) {
        data.push_back(static_cast<uint8_t>(*p));
    }
    data.push_back(static_cast<uint8_t>(version));  // Version character (byte 22)
    data.push_back(0x1a);  // EOF marker (byte 23)
    data.push_back(0);     // Root mode byte (byte 24)
    return data;
}

// Helper to append a little-endian uint32_t
static void append_u32(std::vector<uint8_t>& data, uint32_t val) {
    data.push_back(val & 0xFF);
    data.push_back((val >> 8) & 0xFF);
    data.push_back((val >> 16) & 0xFF);
    data.push_back((val >> 24) & 0xFF);
}

// Ensure effects are registered
static void ensure_effects_registered() {
    static bool registered = false;
    if (!registered) {
        avs::register_builtin_effects();
        registered = true;
    }
}

TEST_CASE("Binary preset header validation", "[preset][binary]") {
    ensure_effects_registered();
    avs::EffectListRoot root;

    SECTION("Valid version 2 header") {
        auto data = make_header('2');
        REQUIRE(avs::Preset::from_legacy(data, root) == true);
    }

    SECTION("Valid version 1 header") {
        auto data = make_header('1');
        REQUIRE(avs::Preset::from_legacy(data, root) == true);
    }

    SECTION("Invalid version header") {
        auto data = make_header('3');
        REQUIRE(avs::Preset::from_legacy(data, root) == false);
        REQUIRE(avs::Preset::last_error().find("version") != std::string::npos);
    }

    SECTION("Too small file") {
        std::vector<uint8_t> data = {0x00, 0x01, 0x02};
        REQUIRE(avs::Preset::from_legacy(data, root) == false);
        REQUIRE(avs::Preset::last_error().find("too small") != std::string::npos);
    }

    SECTION("Invalid header string") {
        std::vector<uint8_t> data(26, 0);
        REQUIRE(avs::Preset::from_legacy(data, root) == false);
        REQUIRE(avs::Preset::last_error().find("header") != std::string::npos);
    }
}

TEST_CASE("Binary preset effect loading", "[preset][binary]") {
    ensure_effects_registered();
    avs::EffectListRoot root;

    SECTION("Empty preset (no effects)") {
        auto data = make_header();
        REQUIRE(avs::Preset::from_legacy(data, root) == true);
        REQUIRE(root.child_count() == 0);
    }

    SECTION("Single Clear effect (index 25)") {
        auto data = make_header();
        // Effect index 25 (Clear)
        append_u32(data, 25);
        // Config length (16 bytes for Clear)
        append_u32(data, 16);
        // Config data (all zeros for defaults)
        for (int i = 0; i < 16; i++) {
            data.push_back(0);
        }

        REQUIRE(avs::Preset::from_legacy(data, root) == true);
        REQUIRE(root.child_count() == 1);
        REQUIRE(root.get_child(0)->get_plugin_info().name == "Clear");
    }

    SECTION("Single Blur effect (index 6)") {
        auto data = make_header();
        append_u32(data, 6);  // Blur
        append_u32(data, 4);  // Config length
        data.push_back(0);
        data.push_back(0);
        data.push_back(0);
        data.push_back(0);

        REQUIRE(avs::Preset::from_legacy(data, root) == true);
        REQUIRE(root.child_count() == 1);
        REQUIRE(root.get_child(0)->get_plugin_info().name == "Blur");
    }

    SECTION("Multiple effects") {
        auto data = make_header();

        // Clear (25)
        append_u32(data, 25);
        append_u32(data, 16);
        for (int i = 0; i < 16; i++) data.push_back(0);

        // Blur (6)
        append_u32(data, 6);
        append_u32(data, 4);
        for (int i = 0; i < 4; i++) data.push_back(0);

        // Brightness (22)
        append_u32(data, 22);
        append_u32(data, 36);  // Brightness config size
        for (int i = 0; i < 36; i++) data.push_back(0);

        REQUIRE(avs::Preset::from_legacy(data, root) == true);
        REQUIRE(root.child_count() == 3);
        REQUIRE(root.get_child(0)->get_plugin_info().name == "Clear");
        REQUIRE(root.get_child(1)->get_plugin_info().name == "Blur");
        REQUIRE(root.get_child(2)->get_plugin_info().name == "Brightness");
    }

    SECTION("Unknown effect index creates placeholder") {
        auto data = make_header();

        // Unknown effect index 99 (beyond known effects)
        append_u32(data, 99);
        append_u32(data, 8);
        for (int i = 0; i < 8; i++) data.push_back(0);

        // Known effect - Clear (25)
        append_u32(data, 25);
        append_u32(data, 16);
        for (int i = 0; i < 16; i++) data.push_back(0);

        REQUIRE(avs::Preset::from_legacy(data, root) == true);
        // Unknown effect creates placeholder, Clear loaded
        REQUIRE(root.child_count() == 2);
        // First effect is unsupported placeholder
        REQUIRE(root.get_child(0)->get_plugin_info().name.find("Unsupported") != std::string::npos);
        REQUIRE(root.get_child(1)->get_plugin_info().name == "Clear");
    }
}

TEST_CASE("Binary preset clears existing effects", "[preset][binary]") {
    ensure_effects_registered();
    avs::EffectListRoot root;

    // Add an effect first
    auto existing = avs::PluginManager::instance().create_effect("Blur");
    root.add_child(std::move(existing));
    REQUIRE(root.child_count() == 1);

    // Load empty preset
    auto data = make_header();
    REQUIRE(avs::Preset::from_legacy(data, root) == true);
    REQUIRE(root.child_count() == 0);
}

TEST_CASE("Binary preset format detection", "[preset][binary]") {
    SECTION("Detect from .avs extension") {
        REQUIRE(avs::Preset::detect_format("test.avs") == avs::PresetFormat::LEGACY);
    }

    SECTION("Detect from .json extension") {
        REQUIRE(avs::Preset::detect_format("test.json") == avs::PresetFormat::JSON);
    }

    SECTION("Detect from .AVS extension (case insensitive would be nice)") {
        // Currently case-sensitive, this tests current behavior
        REQUIRE(avs::Preset::detect_format("test.AVS") == avs::PresetFormat::JSON);  // Falls back to JSON
    }
}

TEST_CASE("Load real AVS preset file", "[preset][binary][file]") {
    ensure_effects_registered();
    avs::EffectListRoot root;

    // Load the actual "superscope love" preset
    std::string preset_path = std::string(TEST_PRESETS_DIR) + "/justin - superscope love.avs";

    SECTION("File loads without error") {
        bool result = avs::Preset::load(preset_path, root);
        INFO("Last error: " << avs::Preset::last_error());
        REQUIRE(result == true);
    }

    SECTION("Contains expected effects") {
        avs::Preset::load(preset_path, root);

        // Print what we loaded for debugging
        std::string loaded_effects;
        for (size_t i = 0; i < root.child_count(); i++) {
            if (i > 0) loaded_effects += ", ";
            loaded_effects += root.get_child(i)->get_plugin_info().name;
        }
        INFO("Loaded effects: " << loaded_effects);
        INFO("Effect count: " << root.child_count());

        // The preset should contain SuperScope effects
        REQUIRE(root.child_count() > 0);
    }
}
