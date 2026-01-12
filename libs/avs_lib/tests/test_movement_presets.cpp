#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "effects/movement.h"

using namespace avs;

TEST_CASE("Movement Effect Script Presets", "[movement]") {
    MovementEffect effect;
    
    SECTION("Preset script extraction") {
        // Test that preset scripts are correctly retrieved
        // Native effects (0, 1, 7) return comment descriptions
        REQUIRE(effect.get_preset_script(0) == "// none - identity transform (native)");
        REQUIRE(effect.get_preset_script(3) == "r = r + (0.1 - (0.2 * d)); d = d * 0.96"); // big swirl out
        REQUIRE(effect.get_preset_script(12) == "r = r + 0.04; d = d * (0.96 + cos(d * $PI) * 0.05)"); // tunneling
    }
    
    SECTION("Default parameters match AVS") {
        auto& params = effect.parameters();

        // Note: Movement effect has no enabled checkbox in original AVS
        REQUIRE(params.get_int("preset") == 0); // none by default
        REQUIRE(params.get_bool("rectangular") == false); // polar by default
        REQUIRE(params.get_bool("wrap") == false);
        REQUIRE(params.get_bool("source_mapped") == false);
        REQUIRE(params.get_bool("subpixel") == true);
    }
}