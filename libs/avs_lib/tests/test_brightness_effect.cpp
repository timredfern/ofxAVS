// Tests for BrightnessEffect lookup tables and rendering
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "effects/brightness_effect.h"
#include "core/plugin_manager.h"
#include <vector>
#include <cstdio>

using namespace avs;

// Helper to extract RGB components
static void extractRGB(uint32_t color, int& r, int& g, int& b) {
    r = (color >> 16) & 0xFF;
    g = (color >> 8) & 0xFF;
    b = color & 0xFF;
}

TEST_CASE("Brightness Effect", "[brightness]") {
    BrightnessEffect effect;
    AudioData dummy_audio = {};

    SECTION("Effect initialization and parameters") {
        REQUIRE(effect.get_plugin_info().name == "Brightness");
        REQUIRE(effect.is_enabled() == true);

        auto& params = effect.parameters();

        // Check default values
        INFO("red_adjust default: " << params.get_int("red_adjust"));
        INFO("green_adjust default: " << params.get_int("green_adjust"));
        INFO("blue_adjust default: " << params.get_int("blue_adjust"));

        REQUIRE(params.get_int("red_adjust") == 4096);
        REQUIRE(params.get_int("green_adjust") == 4096);
        REQUIRE(params.get_int("blue_adjust") == 4096);
        REQUIRE(params.get_bool("additive") == false);
        REQUIRE(params.get_bool("5050") == false);
        REQUIRE(params.get_bool("exclude") == false);
    }

    SECTION("No change at default slider position (4096)") {
        // At slider=4096, brightness should be unchanged
        const int w = 4, h = 4;
        std::vector<uint32_t> framebuffer = {
            0x00000000, 0x00808080, 0x00FFFFFF, 0x00FF0000,
            0x0000FF00, 0x000000FF, 0x00FFFF00, 0x00FF00FF,
            0x0000FFFF, 0x00404040, 0x00C0C0C0, 0x00123456,
            0x00ABCDEF, 0x00FEDCBA, 0x00112233, 0x00778899
        };
        std::vector<uint32_t> original = framebuffer;
        std::vector<uint32_t> fbout(w * h, 0);

        int result = effect.render(dummy_audio, 0, framebuffer.data(), fbout.data(), w, h);

        INFO("Result: " << result);
        for (int i = 0; i < w * h; i++) {
            INFO("Pixel " << i << ": original=0x" << std::hex << original[i]
                 << " after=0x" << framebuffer[i]);
            REQUIRE(framebuffer[i] == original[i]);
        }
    }

    SECTION("Brightness increase (slider > 4096)") {
        auto& params = effect.parameters();

        // Set red to maximum brightness
        params.set_int("red_adjust", 8192);  // Max brightness for red
        params.set_int("green_adjust", 4096); // Normal for green
        params.set_int("blue_adjust", 4096);  // Normal for blue

        const int w = 2, h = 2;
        std::vector<uint32_t> framebuffer = {
            0x00800000, 0x00008000,  // Mid red, mid green
            0x00000080, 0x00808080   // Mid blue, mid gray
        };
        std::vector<uint32_t> original = framebuffer;
        std::vector<uint32_t> fbout(w * h, 0);

        effect.render(dummy_audio, 0, framebuffer.data(), fbout.data(), w, h);

        // Red channel should be brighter (clamped to 255)
        int r0, g0, b0;
        extractRGB(framebuffer[0], r0, g0, b0);
        INFO("Pixel 0: original red=0x80, after red=" << r0);
        REQUIRE(r0 > 0x80);  // Red should be brighter

        // Green-only pixel should be unchanged
        int r1, g1, b1;
        extractRGB(framebuffer[1], r1, g1, b1);
        INFO("Pixel 1: original green=0x80, after green=" << g1);
        REQUIRE(g1 == 0x80);  // Green unchanged
        REQUIRE(r1 == 0);     // Red still 0
    }

    SECTION("Brightness decrease (slider < 4096)") {
        auto& params = effect.parameters();

        // Set all channels to minimum
        params.set_int("red_adjust", 0);
        params.set_int("green_adjust", 0);
        params.set_int("blue_adjust", 0);

        const int w = 2, h = 2;
        std::vector<uint32_t> framebuffer = {
            0x00FFFFFF,  // White
            0x00808080,  // Gray
            0x00FF0000,  // Red
            0x0000FF00   // Green
        };
        std::vector<uint32_t> fbout(w * h, 0);

        effect.render(dummy_audio, 0, framebuffer.data(), fbout.data(), w, h);

        // All pixels should be darker (ideally black at slider=0)
        for (int i = 0; i < w * h; i++) {
            int r, g, b;
            extractRGB(framebuffer[i], r, g, b);
            INFO("Pixel " << i << ": r=" << r << " g=" << g << " b=" << b);
            REQUIRE(r == 0);
            REQUIRE(g == 0);
            REQUIRE(b == 0);
        }
    }

    SECTION("Individual channel control") {
        auto& params = effect.parameters();

        // Boost only green
        params.set_int("red_adjust", 4096);   // Normal
        params.set_int("green_adjust", 8192); // Max
        params.set_int("blue_adjust", 4096);  // Normal

        const int w = 1, h = 1;
        std::vector<uint32_t> framebuffer = { 0x00404040 };  // Dark gray
        std::vector<uint32_t> fbout(w * h, 0);

        effect.render(dummy_audio, 0, framebuffer.data(), fbout.data(), w, h);

        int r, g, b;
        extractRGB(framebuffer[0], r, g, b);
        INFO("After green boost: r=" << r << " g=" << g << " b=" << b);

        REQUIRE(r == 0x40);  // Red unchanged
        REQUIRE(g > 0x40);   // Green boosted
        REQUIRE(b == 0x40);  // Blue unchanged
    }

    SECTION("Disabled effect passes through unchanged") {
        auto& params = effect.parameters();
        params.set_bool("enabled", false);
        params.set_int("red_adjust", 8192);  // Would boost if enabled

        const int w = 2, h = 2;
        std::vector<uint32_t> framebuffer = {
            0x00808080, 0x00FFFFFF,
            0x00000000, 0x00FF00FF
        };
        std::vector<uint32_t> original = framebuffer;
        std::vector<uint32_t> fbout(w * h, 0);

        int result = effect.render(dummy_audio, 0, framebuffer.data(), fbout.data(), w, h);

        REQUIRE(result == 0);
        for (int i = 0; i < w * h; i++) {
            REQUIRE(framebuffer[i] == original[i]);
        }
    }
}

TEST_CASE("Brightness Lookup Table Calculations", "[brightness][lookup]") {
    // Test the lookup table math directly
    // Formula: rm = (1 + (redp < 0 ? 1 : 16) * (redp/4096)) * 65536
    // where redp = slider_value - 4096

    SECTION("Center position (4096) gives multiplier of 1.0") {
        int redp = 4096 - 4096;  // = 0
        float rm = (1.0f + (redp < 0 ? 1 : 16) * ((float)redp / 4096)) * 65536.0f;

        INFO("redp=" << redp << " rm=" << rm);
        REQUIRE(rm == Catch::Approx(65536.0f));  // 1.0 * 65536

        // At multiplier 1.0, lookup table should preserve values
        int red_tab_128 = (128 * (int)rm) & 0xffff0000;
        red_tab_128 = std::min(red_tab_128, 0xff0000);
        INFO("red_tab[128] = 0x" << std::hex << red_tab_128);
        REQUIRE((red_tab_128 >> 16) == 128);
    }

    SECTION("Maximum position (8192) gives 17x multiplier") {
        int redp = 8192 - 4096;  // = 4096
        float rm = (1.0f + (redp < 0 ? 1 : 16) * ((float)redp / 4096)) * 65536.0f;

        INFO("redp=" << redp << " rm=" << rm);
        // rm = (1 + 16 * 1) * 65536 = 17 * 65536
        REQUIRE(rm == Catch::Approx(17.0f * 65536.0f));
    }

    SECTION("Minimum position (0) gives 0x multiplier") {
        int redp = 0 - 4096;  // = -4096
        float rm = (1.0f + (redp < 0 ? 1 : 16) * ((float)redp / 4096)) * 65536.0f;

        INFO("redp=" << redp << " rm=" << rm);
        // rm = (1 + 1 * -1) * 65536 = 0
        REQUIRE(rm == Catch::Approx(0.0f));
    }

    SECTION("Half brightness (2048) gives 0.5x multiplier") {
        int redp = 2048 - 4096;  // = -2048
        float rm = (1.0f + (redp < 0 ? 1 : 16) * ((float)redp / 4096)) * 65536.0f;

        INFO("redp=" << redp << " rm=" << rm);
        // rm = (1 + 1 * -0.5) * 65536 = 0.5 * 65536
        REQUIRE(rm == Catch::Approx(0.5f * 65536.0f));
    }
}
