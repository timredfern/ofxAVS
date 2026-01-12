// Test color loading in load_parameters
// Verifies that binary AVS color values are correctly loaded into effect parameters

#include <catch2/catch_test_macros.hpp>
#include "core/color.h"
#include "core/binary_reader.h"
#include "core/preset.h"
#include "core/plugin_manager.h"
#include "core/builtin_effects.h"
#include "effects/oscilloscope.h"
#include "effects/effect_list_root.h"
#include <vector>
#include <cstdint>

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

TEST_CASE("BinaryReader BGR color conversion", "[color][binary]") {
    // Windows COLORREF format: 0x00BBGGRR
    // Our internal format: 0xAABBGGRR (same byte order, with alpha)

    SECTION("bgr_add_alpha - Orange in Windows format") {
        // Orange = RGB(255, 128, 0)
        // In Windows COLORREF (BGR): 0x000080FF (B=00, G=80, R=FF)
        uint32_t windows_orange = 0x000080FF;

        // After adding alpha, should be: 0xFF0080FF
        // In our ABGR format: A=FF, B=00, G=80, R=FF = orange
        uint32_t expected_internal = 0xFF0080FF;

        // The conversion should preserve byte order, just add alpha
        uint32_t converted = avs::color::bgr_add_alpha(windows_orange);

        // Extract components using our internal format (ABGR)
        uint8_t r = converted & 0xFF;
        uint8_t g = (converted >> 8) & 0xFF;
        uint8_t b = (converted >> 16) & 0xFF;
        uint8_t a = (converted >> 24) & 0xFF;

        INFO("Converted: 0x" << std::hex << converted);
        INFO("Expected:  0x" << std::hex << expected_internal);
        INFO("R=" << (int)r << " G=" << (int)g << " B=" << (int)b << " A=" << (int)a);

        REQUIRE(converted == expected_internal);
        // For orange: R=255, G=128, B=0
        REQUIRE(r == 255);
        REQUIRE(g == 128);
        REQUIRE(b == 0);
        REQUIRE(a == 255);
    }

    SECTION("bgr_add_alpha - Green in Windows format") {
        // Green = RGB(0, 255, 0)
        // In Windows COLORREF: 0x0000FF00 (B=00, G=FF, R=00)
        uint32_t windows_green = 0x0000FF00;

        uint32_t converted = avs::color::bgr_add_alpha(windows_green);

        uint8_t r = converted & 0xFF;
        uint8_t g = (converted >> 8) & 0xFF;
        uint8_t b = (converted >> 16) & 0xFF;

        REQUIRE(r == 0);
        REQUIRE(g == 255);
        REQUIRE(b == 0);
    }

    SECTION("bgr_add_alpha - Purple in Windows format") {
        // Purple = RGB(128, 0, 255)
        // In Windows COLORREF: 0x00FF0080 (B=FF, G=00, R=80)
        uint32_t windows_purple = 0x00FF0080;

        uint32_t converted = avs::color::bgr_add_alpha(windows_purple);

        uint8_t r = converted & 0xFF;
        uint8_t g = (converted >> 8) & 0xFF;
        uint8_t b = (converted >> 16) & 0xFF;

        REQUIRE(r == 128);
        REQUIRE(g == 0);
        REQUIRE(b == 255);
    }

    SECTION("abgr_to_argb - converts internal to JSON format") {
        // Internal ABGR orange: 0xFF0080FF (A=FF, B=00, G=80, R=FF)
        uint32_t internal_orange = 0xFF0080FF;

        // Should convert to ARGB: 0xFFFF8000 (A=FF, R=FF, G=80, B=00)
        uint32_t argb = avs::color::abgr_to_argb(internal_orange);

        REQUIRE(argb == 0xFFFF8000);
    }

    SECTION("argb_to_abgr - converts JSON to internal format") {
        // JSON ARGB orange: 0xFFFF8000 (A=FF, R=FF, G=80, B=00)
        uint32_t json_orange = 0xFFFF8000;

        // Should convert to internal ABGR: 0xFF0080FF (A=FF, B=00, G=80, R=FF)
        uint32_t internal = avs::color::argb_to_abgr(json_orange);

        REQUIRE(internal == 0xFF0080FF);
    }

    SECTION("Round-trip: ABGR -> ARGB -> ABGR") {
        uint32_t colors[] = {0xFF0080FF, 0xFF00FF00, 0xFFFF0080, 0xFFFFFFFF, 0xFF000000};
        for (uint32_t c : colors) {
            uint32_t argb = avs::color::abgr_to_argb(c);
            uint32_t back = avs::color::argb_to_abgr(argb);
            REQUIRE(back == c);
        }
    }
}

TEST_CASE("Oscilloscope load_parameters color loading", "[color][oscilloscope]") {
    ensure_effects_registered();

    SECTION("Single orange color") {
        avs::OscilloscopeEffect effect;

        // Build binary config data:
        // effect (int32) - mode bits
        // num_colors (int32) = 1
        // colors[0] (int32) = orange in BGR format
        std::vector<uint8_t> data;
        append_u32(data, 0);           // effect mode bits
        append_u32(data, 1);           // num_colors = 1
        append_u32(data, 0x000080FF);  // Orange in Windows BGR format

        effect.load_parameters(data);

        uint32_t loaded_color = effect.parameters().get_color("color_0");

        // Extract RGB from loaded color (using internal ABGR format)
        uint8_t r = loaded_color & 0xFF;
        uint8_t g = (loaded_color >> 8) & 0xFF;
        uint8_t b = (loaded_color >> 16) & 0xFF;

        INFO("Loaded color: 0x" << std::hex << loaded_color);
        INFO("R=" << (int)r << " G=" << (int)g << " B=" << (int)b);

        // Orange should be R=255, G=128, B=0
        REQUIRE(r == 255);
        REQUIRE(g == 128);
        REQUIRE(b == 0);
    }

    SECTION("Three colors: orange, green, purple") {
        avs::OscilloscopeEffect effect;

        std::vector<uint8_t> data;
        append_u32(data, 0);           // effect mode bits
        append_u32(data, 3);           // num_colors = 3
        append_u32(data, 0x000080FF);  // Orange: RGB(255, 128, 0) in BGR
        append_u32(data, 0x0000FF00);  // Green: RGB(0, 255, 0) in BGR
        append_u32(data, 0x00FF0080);  // Purple: RGB(128, 0, 255) in BGR

        effect.load_parameters(data);

        // Check color 0 (orange)
        uint32_t c0 = effect.parameters().get_color("color_0");
        REQUIRE((c0 & 0xFF) == 255);         // R
        REQUIRE(((c0 >> 8) & 0xFF) == 128);  // G
        REQUIRE(((c0 >> 16) & 0xFF) == 0);   // B

        // Check color 1 (green)
        uint32_t c1 = effect.parameters().get_color("color_1");
        REQUIRE((c1 & 0xFF) == 0);           // R
        REQUIRE(((c1 >> 8) & 0xFF) == 255);  // G
        REQUIRE(((c1 >> 16) & 0xFF) == 0);   // B

        // Check color 2 (purple)
        uint32_t c2 = effect.parameters().get_color("color_2");
        REQUIRE((c2 & 0xFF) == 128);         // R
        REQUIRE(((c2 >> 8) & 0xFF) == 0);    // G
        REQUIRE(((c2 >> 16) & 0xFF) == 255); // B
    }
}

TEST_CASE("JSON color serialization", "[color][json]") {
    ensure_effects_registered();

    SECTION("JSON output shows ARGB format") {
        // Create effect with orange color (internal ABGR: 0xFF0080FF)
        avs::OscilloscopeEffect effect;
        std::vector<uint8_t> data;
        append_u32(data, 0);           // mode
        append_u32(data, 1);           // num_colors
        append_u32(data, 0x000080FF);  // Orange in BGR
        effect.load_parameters(data);

        // Serialize to JSON
        avs::EffectListRoot root;
        root.add_child(std::make_unique<avs::OscilloscopeEffect>(effect));
        std::string json = avs::Preset::to_json(root);

        // JSON should contain ARGB orange: #FFFF8000
        INFO("JSON: " << json);
        REQUIRE(json.find("#FFFF8000") != std::string::npos);
        // Should NOT contain ABGR format
        REQUIRE(json.find("#FF0080FF") == std::string::npos);
    }

    SECTION("JSON round-trip preserves colors") {
        // Create effect with known colors
        avs::OscilloscopeEffect effect;
        std::vector<uint8_t> data;
        append_u32(data, 0);           // mode
        append_u32(data, 3);           // num_colors
        append_u32(data, 0x000080FF);  // Orange in BGR
        append_u32(data, 0x0000FF00);  // Green in BGR
        append_u32(data, 0x00FF0080);  // Purple in BGR
        effect.load_parameters(data);

        // Get original colors
        uint32_t orig_c0 = effect.parameters().get_color("color_0");
        uint32_t orig_c1 = effect.parameters().get_color("color_1");
        uint32_t orig_c2 = effect.parameters().get_color("color_2");

        // Serialize to JSON
        avs::EffectListRoot root;
        root.add_child(std::make_unique<avs::OscilloscopeEffect>(effect));
        std::string json = avs::Preset::to_json(root);

        INFO("JSON: " << json);

        // Load back from JSON
        avs::EffectListRoot root2;
        REQUIRE(avs::Preset::from_json(json, root2) == true);
        REQUIRE(root2.child_count() == 1);

        // Get loaded colors
        auto* loaded = root2.get_child(0);
        uint32_t loaded_c0 = loaded->parameters().get_color("color_0");
        uint32_t loaded_c1 = loaded->parameters().get_color("color_1");
        uint32_t loaded_c2 = loaded->parameters().get_color("color_2");

        // Colors should match original internal values
        REQUIRE(loaded_c0 == orig_c0);
        REQUIRE(loaded_c1 == orig_c1);
        REQUIRE(loaded_c2 == orig_c2);

        // Verify actual RGB values
        REQUIRE(avs::color::red(loaded_c0) == 255);   // Orange R
        REQUIRE(avs::color::green(loaded_c0) == 128); // Orange G
        REQUIRE(avs::color::blue(loaded_c0) == 0);    // Orange B

        REQUIRE(avs::color::red(loaded_c1) == 0);     // Green R
        REQUIRE(avs::color::green(loaded_c1) == 255); // Green G
        REQUIRE(avs::color::blue(loaded_c1) == 0);    // Green B

        REQUIRE(avs::color::red(loaded_c2) == 128);   // Purple R
        REQUIRE(avs::color::green(loaded_c2) == 0);   // Purple G
        REQUIRE(avs::color::blue(loaded_c2) == 255);  // Purple B
    }
}
