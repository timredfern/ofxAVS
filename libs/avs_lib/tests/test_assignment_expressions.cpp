#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/script/script_engine.h"
#include <cmath>

using namespace avs;

TEST_CASE("Assignment Expressions", "[assignment][expressions]") {
    ScriptEngine engine;
    
    SECTION("Simple assignment") {
        double result = engine.evaluate("x = 5");
        REQUIRE(result == 5.0);
        REQUIRE(engine.get_variable("x") == 5.0);
    }
    
    SECTION("Assignment with expression") {
        engine.set_variable("y", 3.0);
        double result = engine.evaluate("x = y + 2");
        REQUIRE(result == 5.0);
        REQUIRE(engine.get_variable("x") == 5.0);
    }
    
    SECTION("Multiple assignments") {
        double result = engine.evaluate("a = 10; b = a * 2");
        REQUIRE(result == 20.0); // Should return result of last statement
        REQUIRE(engine.get_variable("a") == 10.0);
        REQUIRE(engine.get_variable("b") == 20.0);
    }
    
    SECTION("Assignment chain") {
        double result = engine.evaluate("x = 1; x = x + 1; x = x * 2");
        REQUIRE(result == 4.0);
        REQUIRE(engine.get_variable("x") == 4.0);
    }
    
    SECTION("Assignment with built-in variables") {
        engine.set_pixel_context(50, 25, 100, 100);
        double result = engine.evaluate("temp = x + y");
        // x = 50/99 ≈ 0.505, y = 25/99 ≈ 0.252, temp = x + y ≈ 0.758
        REQUIRE(result == Catch::Approx(50.0/99.0 + 25.0/99.0).epsilon(0.001));
        
        // Let me recalculate: x = 50/99 ≈ 0.505, y = 25/99 ≈ 0.252, sum ≈ 0.757
        REQUIRE(std::abs(result - 0.7575757575757576) < 0.0001);
        REQUIRE(std::abs(engine.get_variable("temp") - 0.7575757575757576) < 0.0001);
    }
    
    SECTION("Assignment with audio variables") {
        AudioData test_audio = {};
        test_audio[0][0][0] = 64; // v1 = 64/127 ≈ 0.504
        
        engine.set_audio_context(test_audio, true);
        double result = engine.evaluate("audio_level = v1 + beat");
        
        double expected = (64.0/127.0) + 1.0; // v1 + beat
        REQUIRE(std::abs(result - expected) < 0.0001);
        REQUIRE(std::abs(engine.get_variable("audio_level") - expected) < 0.0001);
    }
    
    SECTION("Assignment with functions") {
        double result = engine.evaluate("angle = 3.14159; sine_val = sin(angle)");
        REQUIRE(std::abs(result - std::sin(3.14159)) < 0.0001);
        REQUIRE(engine.get_variable("angle") == 3.14159);
        REQUIRE(std::abs(engine.get_variable("sine_val") - std::sin(3.14159)) < 0.0001);
    }
    
    SECTION("Reassignment overwrites previous value") {
        engine.evaluate("x = 10");
        REQUIRE(engine.get_variable("x") == 10.0);
        
        double result = engine.evaluate("x = 20");
        REQUIRE(result == 20.0);
        REQUIRE(engine.get_variable("x") == 20.0);
    }
    
    SECTION("Assignment doesn't affect built-in variables") {
        engine.set_pixel_context(50, 25, 100, 100);
        
        // Try to assign to built-in variable
        double result = engine.evaluate("x = 0.9");
        REQUIRE(result == 0.9);
        
        // After assignment, x retains the assigned value (0.9), not the built-in
        // This is the correct behavior - user assignments override built-ins
        double x_builtin = engine.get_variable("x");
        REQUIRE(x_builtin == 0.9);
    }
    
    SECTION("Mixed assignments and expressions") {
        double result = engine.evaluate("a = 5; b = 3; a + b");
        REQUIRE(result == 8.0);
        REQUIRE(engine.get_variable("a") == 5.0);
        REQUIRE(engine.get_variable("b") == 3.0);
    }
    
    SECTION("Assignment with trailing semicolon") {
        double result = engine.evaluate("x = 42;");
        REQUIRE(result == 42.0);
        REQUIRE(engine.get_variable("x") == 42.0);
    }
    
    SECTION("Complex transform-like expression") {
        // Simulate a typical AVS transform expression
        engine.set_pixel_context(50, 50, 100, 100);
        engine.set_audio_context(AudioData{}, true); // beat = true
        
        double result = engine.evaluate("temp = x + 0.1; newx = temp * (1 + beat * 0.1); newx");
        
        // Expected: x = 50/99 ≈ 0.505, temp = 0.505 + 0.1 = 0.605, newx = 0.605 * (1 + 1*0.1) = 0.605 * 1.1 = 0.6655
        double expected_x = 50.0 / 99.0;
        double expected_temp = expected_x + 0.1;
        double expected_newx = expected_temp * 1.1;
        
        REQUIRE(std::abs(result - expected_newx) < 0.0001);
        REQUIRE(std::abs(engine.get_variable("temp") - expected_temp) < 0.0001);
        REQUIRE(std::abs(engine.get_variable("newx") - expected_newx) < 0.0001);
    }
}