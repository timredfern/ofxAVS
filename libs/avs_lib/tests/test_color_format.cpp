// Test color format consistency across the codebase
// AVS uses 0xAABBGGRR format:
// - Red in bits 0-7
// - Green in bits 8-15
// - Blue in bits 16-23
// - Alpha in bits 24-31
//
// This matches OF_PIXELS_BGRA on little-endian systems.

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

// Define the canonical 0xAABBGGRR color constants
namespace avs_color {
    // Format: 0xAABBGGRR
    constexpr uint32_t RED   = 0xFF0000FF;  // A=FF, B=00, G=00, R=FF
    constexpr uint32_t GREEN = 0xFF00FF00;  // A=FF, B=00, G=FF, R=00
    constexpr uint32_t BLUE  = 0xFFFF0000;  // A=FF, B=FF, G=00, R=00
    constexpr uint32_t WHITE = 0xFFFFFFFF;  // A=FF, B=FF, G=FF, R=FF
    constexpr uint32_t BLACK = 0xFF000000;  // A=FF, B=00, G=00, R=00

    // Extract components from 0xAABBGGRR color
    inline uint8_t red(uint32_t color)   { return color & 0xFF; }
    inline uint8_t green(uint32_t color) { return (color >> 8) & 0xFF; }
    inline uint8_t blue(uint32_t color)  { return (color >> 16) & 0xFF; }
    inline uint8_t alpha(uint32_t color) { return (color >> 24) & 0xFF; }

    // Build 0xAABBGGRR color from components
    inline uint32_t make(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return static_cast<uint32_t>(r) |
               (static_cast<uint32_t>(g) << 8) |
               (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(a) << 24);
    }
}

TEST_CASE("0xAABBGGRR color format constants", "[color]") {
    SECTION("Red is 0xFF0000FF (R in low byte)") {
        REQUIRE(avs_color::RED == 0xFF0000FF);
        REQUIRE(avs_color::red(avs_color::RED) == 255);
        REQUIRE(avs_color::green(avs_color::RED) == 0);
        REQUIRE(avs_color::blue(avs_color::RED) == 0);
        REQUIRE(avs_color::alpha(avs_color::RED) == 255);
    }

    SECTION("Green is 0xFF00FF00") {
        REQUIRE(avs_color::GREEN == 0xFF00FF00);
        REQUIRE(avs_color::red(avs_color::GREEN) == 0);
        REQUIRE(avs_color::green(avs_color::GREEN) == 255);
        REQUIRE(avs_color::blue(avs_color::GREEN) == 0);
    }

    SECTION("Blue is 0xFFFF0000 (B in bits 16-23)") {
        REQUIRE(avs_color::BLUE == 0xFFFF0000);
        REQUIRE(avs_color::red(avs_color::BLUE) == 0);
        REQUIRE(avs_color::green(avs_color::BLUE) == 0);
        REQUIRE(avs_color::blue(avs_color::BLUE) == 255);
    }

    SECTION("White is 0xFFFFFFFF") {
        REQUIRE(avs_color::WHITE == 0xFFFFFFFF);
        REQUIRE(avs_color::red(avs_color::WHITE) == 255);
        REQUIRE(avs_color::green(avs_color::WHITE) == 255);
        REQUIRE(avs_color::blue(avs_color::WHITE) == 255);
    }
}

TEST_CASE("0xAABBGGRR color construction", "[color]") {
    SECTION("make() builds correct values") {
        REQUIRE(avs_color::make(255, 0, 0) == avs_color::RED);
        REQUIRE(avs_color::make(0, 255, 0) == avs_color::GREEN);
        REQUIRE(avs_color::make(0, 0, 255) == avs_color::BLUE);
        REQUIRE(avs_color::make(255, 255, 255) == avs_color::WHITE);
        REQUIRE(avs_color::make(0, 0, 0) == avs_color::BLACK);
    }

    SECTION("Round-trip extract and rebuild") {
        uint32_t colors[] = {avs_color::RED, avs_color::GREEN, avs_color::BLUE, 0xFF123456, 0x80AABBCC};
        for (uint32_t c : colors) {
            uint32_t rebuilt = avs_color::make(
                avs_color::red(c), avs_color::green(c), avs_color::blue(c), avs_color::alpha(c)
            );
            REQUIRE(rebuilt == c);
        }
    }
}

TEST_CASE("SuperScope color extraction pattern", "[color][superscope]") {
    // This tests the exact pattern used in SuperScope to extract RGB for script variables:
    // engine_.set_variable("red", (current_color & 0xFF) / 255.0);
    // engine_.set_variable("green", ((current_color >> 8) & 0xFF) / 255.0);
    // engine_.set_variable("blue", ((current_color >> 16) & 0xFF) / 255.0);

    SECTION("Red color extracts correctly") {
        uint32_t current_color = avs_color::RED;  // 0xFF0000FF
        double red = (current_color & 0xFF) / 255.0;
        double green = ((current_color >> 8) & 0xFF) / 255.0;
        double blue = ((current_color >> 16) & 0xFF) / 255.0;

        REQUIRE(red == 1.0);
        REQUIRE(green == 0.0);
        REQUIRE(blue == 0.0);
    }

    SECTION("Green color extracts correctly") {
        uint32_t current_color = avs_color::GREEN;  // 0xFF00FF00
        double red = (current_color & 0xFF) / 255.0;
        double green = ((current_color >> 8) & 0xFF) / 255.0;
        double blue = ((current_color >> 16) & 0xFF) / 255.0;

        REQUIRE(red == 0.0);
        REQUIRE(green == 1.0);
        REQUIRE(blue == 0.0);
    }

    SECTION("Blue color extracts correctly") {
        uint32_t current_color = avs_color::BLUE;  // 0xFFFF0000
        double red = (current_color & 0xFF) / 255.0;
        double green = ((current_color >> 8) & 0xFF) / 255.0;
        double blue = ((current_color >> 16) & 0xFF) / 255.0;

        REQUIRE(red == 0.0);
        REQUIRE(green == 0.0);
        REQUIRE(blue == 1.0);
    }
}

TEST_CASE("SuperScope point_color construction pattern", "[color][superscope]") {
    // This tests the exact pattern used in SuperScope to build point_color:
    // int point_color = make_color_component(red) |
    //                   (make_color_component(green) << 8) |
    //                   (make_color_component(blue) << 16);
    // point_color |= 0xFF000000;  // Alpha

    auto make_color_component = [](double val) -> int {
        int v = static_cast<int>(val * 255.0);
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        return v;
    };

    SECTION("Red renders as red") {
        double red = 1.0, green = 0.0, blue = 0.0;
        int point_color = make_color_component(red) |
                          (make_color_component(green) << 8) |
                          (make_color_component(blue) << 16);
        point_color |= 0xFF000000;

        REQUIRE(static_cast<uint32_t>(point_color) == avs_color::RED);
    }

    SECTION("Green renders as green") {
        double red = 0.0, green = 1.0, blue = 0.0;
        int point_color = make_color_component(red) |
                          (make_color_component(green) << 8) |
                          (make_color_component(blue) << 16);
        point_color |= 0xFF000000;

        REQUIRE(static_cast<uint32_t>(point_color) == avs_color::GREEN);
    }

    SECTION("Blue renders as blue") {
        double red = 0.0, green = 0.0, blue = 1.0;
        int point_color = make_color_component(red) |
                          (make_color_component(green) << 8) |
                          (make_color_component(blue) << 16);
        point_color |= 0xFF000000;

        REQUIRE(static_cast<uint32_t>(point_color) == avs_color::BLUE);
    }
}

TEST_CASE("AVSui color conversion patterns", "[color][ui]") {
    // This tests the exact patterns used in AVSui.cpp

    // Extract ImGui float array from 0xAABBGGRR color
    auto color_to_imgui = [](uint32_t color, float* col) {
        col[0] = (color & 0xFF) / 255.0f;          // R from bits 0-7
        col[1] = ((color >> 8) & 0xFF) / 255.0f;   // G from bits 8-15
        col[2] = ((color >> 16) & 0xFF) / 255.0f;  // B from bits 16-23
        col[3] = ((color >> 24) & 0xFF) / 255.0f;  // A from bits 24-31
    };

    // Build 0xAABBGGRR color from ImGui float array
    auto imgui_to_color = [](const float* col) -> uint32_t {
        return ((uint32_t)(col[3] * 255) << 24) |  // A to bits 24-31
               ((uint32_t)(col[2] * 255) << 16) |  // B to bits 16-23
               ((uint32_t)(col[1] * 255) << 8) |   // G to bits 8-15
               ((uint32_t)(col[0] * 255));         // R to bits 0-7
    };

    SECTION("Red round-trips correctly through UI") {
        float col[4];
        color_to_imgui(avs_color::RED, col);

        REQUIRE(col[0] == 1.0f);  // R
        REQUIRE(col[1] == 0.0f);  // G
        REQUIRE(col[2] == 0.0f);  // B
        REQUIRE(col[3] == 1.0f);  // A

        uint32_t rebuilt = imgui_to_color(col);
        REQUIRE(rebuilt == avs_color::RED);
    }

    SECTION("Green round-trips correctly through UI") {
        float col[4];
        color_to_imgui(avs_color::GREEN, col);

        REQUIRE(col[0] == 0.0f);  // R
        REQUIRE(col[1] == 1.0f);  // G
        REQUIRE(col[2] == 0.0f);  // B

        uint32_t rebuilt = imgui_to_color(col);
        REQUIRE(rebuilt == avs_color::GREEN);
    }

    SECTION("Blue round-trips correctly through UI") {
        float col[4];
        color_to_imgui(avs_color::BLUE, col);

        REQUIRE(col[0] == 0.0f);  // R
        REQUIRE(col[1] == 0.0f);  // G
        REQUIRE(col[2] == 1.0f);  // B

        uint32_t rebuilt = imgui_to_color(col);
        REQUIRE(rebuilt == avs_color::BLUE);
    }
}

TEST_CASE("Color parameter storage format", "[color][preset]") {
    // Colors stored in presets must be in 0xAABBGGRR format

    SECTION("Red stored as 4278190335 (0xFF0000FF)") {
        uint32_t red_color = 0xFF0000FF;
        REQUIRE(red_color == 4278190335);
        REQUIRE(avs_color::red(red_color) == 255);
        REQUIRE(avs_color::green(red_color) == 0);
        REQUIRE(avs_color::blue(red_color) == 0);
    }

    SECTION("White with alpha stored as 4294967295 (0xFFFFFFFF)") {
        uint32_t white_color = 0xFFFFFFFF;
        REQUIRE(white_color == 4294967295);
    }

    SECTION("WRONG format: 0xFFFF0000 is BLUE not RED") {
        // This documents the WRONG format to avoid confusion
        uint32_t wrong_red = 0xFFFF0000;  // Someone might think this is red - it's NOT!
        REQUIRE(avs_color::red(wrong_red) == 0);    // No red!
        REQUIRE(avs_color::blue(wrong_red) == 255); // It's actually blue!
    }
}

TEST_CASE("Full UI to render pipeline", "[color][integration]") {
    // Simulate the full pipeline: user picks color in UI -> stored -> read by effect -> rendered

    // UI conversion functions
    auto imgui_to_color = [](const float* col) -> uint32_t {
        return ((uint32_t)(col[3] * 255) << 24) |
               ((uint32_t)(col[2] * 255) << 16) |
               ((uint32_t)(col[1] * 255) << 8) |
               ((uint32_t)(col[0] * 255));
    };

    // SuperScope render function
    auto make_color_component = [](double val) -> int {
        int v = static_cast<int>(val * 255.0);
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        return v;
    };

    SECTION("User picks red -> renders red") {
        // Step 1: User picks red in ImGui color picker
        float imgui_col[4] = {1.0f, 0.0f, 0.0f, 1.0f};  // R, G, B, A

        // Step 2: UI converts to storage format
        uint32_t stored_color = imgui_to_color(imgui_col);

        // Step 3: SuperScope reads and extracts
        double red = (stored_color & 0xFF) / 255.0;
        double green = ((stored_color >> 8) & 0xFF) / 255.0;
        double blue = ((stored_color >> 16) & 0xFF) / 255.0;

        // Step 4: SuperScope builds render color
        int point_color = make_color_component(red) |
                          (make_color_component(green) << 8) |
                          (make_color_component(blue) << 16);
        point_color |= 0xFF000000;

        // Verify: rendered color should be red
        REQUIRE(static_cast<uint32_t>(point_color) == avs_color::RED);
    }

    SECTION("User picks blue -> renders blue") {
        float imgui_col[4] = {0.0f, 0.0f, 1.0f, 1.0f};
        uint32_t stored_color = imgui_to_color(imgui_col);

        double red = (stored_color & 0xFF) / 255.0;
        double green = ((stored_color >> 8) & 0xFF) / 255.0;
        double blue = ((stored_color >> 16) & 0xFF) / 255.0;

        int point_color = make_color_component(red) |
                          (make_color_component(green) << 8) |
                          (make_color_component(blue) << 16);
        point_color |= 0xFF000000;

        REQUIRE(static_cast<uint32_t>(point_color) == avs_color::BLUE);
    }
}
