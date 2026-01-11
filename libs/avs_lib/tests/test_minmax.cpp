#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/script/script_engine.h"

TEST_CASE("Script min/max functions", "[script]") {
    avs::ScriptEngine engine;

    SECTION("min with two arguments") {
        engine.evaluate("x = min(5, 3)");
        REQUIRE(engine.get_variable("x") == 3.0);
    }

    SECTION("max with two arguments") {
        engine.evaluate("x = max(5, 3)");
        REQUIRE(engine.get_variable("x") == 5.0);
    }

    SECTION("min with variables") {
        engine.set_variable("a", 10.0);
        engine.set_variable("b", 20.0);
        engine.evaluate("x = min(a, b)");
        REQUIRE(engine.get_variable("x") == 10.0);
    }

    SECTION("max with variables") {
        engine.set_variable("a", 10.0);
        engine.set_variable("b", 20.0);
        engine.evaluate("x = max(a, b)");
        REQUIRE(engine.get_variable("x") == 20.0);
    }

    SECTION("min with negative numbers") {
        engine.evaluate("x = min(-5, 3)");
        REQUIRE(engine.get_variable("x") == -5.0);
    }

    SECTION("max with negative numbers") {
        engine.evaluate("x = max(-5, 3)");
        REQUIRE(engine.get_variable("x") == 3.0);
    }

    SECTION("clamping pattern - max(min(x, upper), lower)") {
        engine.set_variable("lw", 300.0);  // above max
        engine.evaluate("lw = max(1, min(lw, 255))");
        REQUIRE(engine.get_variable("lw") == 255.0);

        engine.set_variable("lw", -5.0);  // below min
        engine.evaluate("lw = max(1, min(lw, 255))");
        REQUIRE(engine.get_variable("lw") == 1.0);

        engine.set_variable("lw", 50.0);  // in range
        engine.evaluate("lw = max(1, min(lw, 255))");
        REQUIRE(engine.get_variable("lw") == 50.0);
    }

    SECTION("nested min/max") {
        engine.evaluate("x = min(max(5, 10), 8)");
        REQUIRE(engine.get_variable("x") == 8.0);
    }

    SECTION("min/max with expressions") {
        engine.set_variable("a", 5.0);
        engine.evaluate("x = min(a + 3, a * 2)");
        REQUIRE(engine.get_variable("x") == 8.0);  // min(8, 10) = 8
    }

    SECTION("min/max in multi-statement script") {
        engine.set_variable("lw", 1.0);
        engine.set_variable("dir", 1.0);
        engine.evaluate("lw = lw + dir; lw = min(lw, 10); lw = max(lw, 1)");
        REQUIRE(engine.get_variable("lw") == 2.0);
    }

    SECTION("render mode style clamping") {
        // Simulate what Set Render Mode does
        engine.set_variable("lw", 5.0);
        engine.set_variable("bm", 3.0);
        engine.set_variable("a", 128.0);

        // Frame script that modifies values
        engine.evaluate("lw = lw + 1");
        engine.evaluate("bm = min(9, max(0, bm))");  // clamp blend mode
        engine.evaluate("a = min(255, max(0, a))");   // clamp alpha

        REQUIRE(engine.get_variable("lw") == 6.0);
        REQUIRE(engine.get_variable("bm") == 3.0);
        REQUIRE(engine.get_variable("a") == 128.0);
    }
}

TEST_CASE("Set Render Mode script pattern", "[script]") {
    avs::ScriptEngine engine;
    
    // Simulate the user's exact script
    SECTION("lw decreasing with max clamp") {
        // Init: dir=1; lw=10
        engine.evaluate("dir=1; lw=10");
        REQUIRE(engine.get_variable("dir") == 1.0);
        REQUIRE(engine.get_variable("lw") == 10.0);
        
        // Frame: lw=max(1,lw-dir)
        engine.evaluate("lw=max(1,lw-dir)");
        REQUIRE(engine.get_variable("lw") == 9.0);
        
        engine.evaluate("lw=max(1,lw-dir)");
        REQUIRE(engine.get_variable("lw") == 8.0);
        
        // Continue until we hit the floor
        for (int i = 0; i < 10; i++) {
            engine.evaluate("lw=max(1,lw-dir)");
        }
        REQUIRE(engine.get_variable("lw") == 1.0);  // Should clamp at 1
        
        // Beat: lw=10
        engine.evaluate("lw=10");
        REQUIRE(engine.get_variable("lw") == 10.0);
    }
}
