#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/script/script_engine.h"

using namespace avs;

// ============================================================================
// MULTI-PHASE SCRIPT EXECUTION TESTS
// ============================================================================

TEST_CASE("Multi-Phase Script Execution", "[script][multiphase]") {
    
    SECTION("Variable persistence across phases") {
        ScriptEngine init_engine, frame_engine, pixel_engine;
        
        // Simulate init phase - set up persistent variables
        init_engine.evaluate("speed = 0.1; counter = 0");
        
        // Transfer variables to frame phase  
        frame_engine.set_variable("speed", init_engine.get_variable("speed"));
        frame_engine.set_variable("counter", init_engine.get_variable("counter"));
        
        // Simulate frame phase - update per-frame variables
        frame_engine.evaluate("counter = counter + 1; time = counter * 0.016");
        
        // Transfer variables to pixel phase
        pixel_engine.set_variable("speed", frame_engine.get_variable("speed"));
        pixel_engine.set_variable("time", frame_engine.get_variable("time"));
        pixel_engine.set_pixel_context(100, 100, 200, 200);
        
        // Simulate pixel phase - transform coordinates
        pixel_engine.evaluate("d = d * (1.0 - speed); r = r + time");
        
        // Verify variable persistence and computation
        REQUIRE(pixel_engine.get_variable("speed") == Catch::Approx(0.1));
        REQUIRE(pixel_engine.get_variable("time") == Catch::Approx(0.016));
        REQUIRE(std::isfinite(pixel_engine.get_variable("d")));
        REQUIRE(std::isfinite(pixel_engine.get_variable("r")));
    }
    
    SECTION("Classic Dynamic Movement phases") {
        ScriptEngine init_engine, frame_engine, beat_engine, pixel_engine;
        
        // Init phase - typical Dynamic Movement initialization  
        init_engine.evaluate(R"(
            c = 200; f = 0; dt = 0; dl = 0; 
            beatdiv = 4; speed = 0.15
        )");
        
        // Frame phase - typical per-frame updates
        frame_engine.set_variable("f", init_engine.get_variable("f"));
        frame_engine.set_variable("beatdiv", init_engine.get_variable("beatdiv"));
        frame_engine.evaluate(R"(
            f = f + 1;
            t = (f * 3.14159 * 2) / beatdiv;
            dt = dl + t;
            dr = cos(dt) * speed * 2;
            dd = 1 - (sin(dt) * speed)
        )");
        
        // Beat phase - beat-reactive changes
        beat_engine.set_variable("speed", init_engine.get_variable("speed"));
        beat_engine.evaluate("speed = speed * 1.2"); // Beat boost
        
        // Pixel phase - coordinate transformation
        pixel_engine.set_variable("dr", frame_engine.get_variable("dr"));
        pixel_engine.set_variable("dd", frame_engine.get_variable("dd"));
        pixel_engine.set_pixel_context(120, 80, 200, 200);
        pixel_engine.evaluate(R"(
            r = r + dr;
            d = d * dd
        )");
        
        // Verify all phases executed correctly
        REQUIRE(frame_engine.get_variable("f") == 1.0);
        REQUIRE(beat_engine.get_variable("speed") == Catch::Approx(0.18)); // 0.15 * 1.2
        REQUIRE(std::isfinite(pixel_engine.get_variable("r")));
        REQUIRE(pixel_engine.get_variable("d") >= 0.0);
    }
    
    SECTION("SuperScope multi-phase rendering") {
        ScriptEngine init_engine, frame_engine, point_engine;
        
        // Init phase - set up rendering parameters
        init_engine.evaluate(R"(
            n = 800; xscale = 2.0; yscale = 1.5;
            red = 1.0; green = 1.0; blue = 1.0
        )");
        
        // Frame phase - update animation variables  
        frame_engine.set_variable("xscale", init_engine.get_variable("xscale"));
        frame_engine.set_variable("yscale", init_engine.get_variable("yscale"));
        frame_engine.evaluate("xscale = xscale * 1.01; yscale = yscale * 0.99");
        
        // Point phase - render each audio sample
        for (int i = 0; i < 10; i++) {
            point_engine.set_variable("n", init_engine.get_variable("n"));
            point_engine.set_variable("xscale", frame_engine.get_variable("xscale"));
            point_engine.set_variable("yscale", frame_engine.get_variable("yscale"));
            point_engine.set_variable("i", i);
            point_engine.set_variable("v", 0.1 * sin(i * 0.1)); // Mock audio sample
            
            point_engine.evaluate(R"(
                x = (i / n - 0.5) * xscale;
                y = v * yscale;
                alpha = abs(v)
            )");
            
            // Verify point was rendered correctly
            REQUIRE(std::abs(point_engine.get_variable("x")) <= 1.01); // Within scaled range
            REQUIRE(std::abs(point_engine.get_variable("y")) <= 1.5 * 0.1); // Within scaled amplitude
            REQUIRE(point_engine.get_variable("alpha") >= 0.0);
        }
        
        // Verify frame animation worked
        REQUIRE(frame_engine.get_variable("xscale") > init_engine.get_variable("xscale"));
        REQUIRE(frame_engine.get_variable("yscale") < init_engine.get_variable("yscale"));
    }
}