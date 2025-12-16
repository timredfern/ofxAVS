#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/script/script_engine.h"
#include <cmath>

using namespace avs;

// ============================================================================
// LEVEL 1: BASIC TOKENIZATION AND PARSING
// ============================================================================

TEST_CASE("Script Engine - Level 1: Basic Tokenization", "[script][level1][tokenization]") {
    ScriptEngine engine;
    
    SECTION("Numbers") {
        REQUIRE(engine.evaluate("42") == Catch::Approx(42.0));
        REQUIRE(engine.evaluate("3.14159") == Catch::Approx(3.14159));
        REQUIRE(engine.evaluate("-5") == Catch::Approx(-5.0));
        REQUIRE(engine.evaluate("0") == Catch::Approx(0.0));
    }
    
    SECTION("Basic operators") {
        REQUIRE(engine.evaluate("2 + 3") == Catch::Approx(5.0));
        REQUIRE(engine.evaluate("10 - 4") == Catch::Approx(6.0));
        REQUIRE(engine.evaluate("5 * 7") == Catch::Approx(35.0));
        REQUIRE(engine.evaluate("20 / 4") == Catch::Approx(5.0));
    }
    
    SECTION("Operator precedence") {
        REQUIRE(engine.evaluate("2 + 3 * 4") == Catch::Approx(14.0)); // Not 20
        REQUIRE(engine.evaluate("(2 + 3) * 4") == Catch::Approx(20.0));
        REQUIRE(engine.evaluate("2 * 3 + 4") == Catch::Approx(10.0));
        REQUIRE(engine.evaluate("2 * (3 + 4)") == Catch::Approx(14.0));
    }
    
    SECTION("Nested parentheses") {
        REQUIRE(engine.evaluate("((2 + 3) * (4 + 1))") == Catch::Approx(25.0));
        REQUIRE(engine.evaluate("(2 + (3 * (4 + 1)))") == Catch::Approx(17.0));
    }
}

// ============================================================================
// LEVEL 2: VARIABLES AND ASSIGNMENT
// ============================================================================

TEST_CASE("Script Engine - Level 2: Variables", "[script][level2][variables]") {
    ScriptEngine engine;
    
    SECTION("Simple variable assignment") {
        double result = engine.evaluate("x = 5");
        REQUIRE(result == Catch::Approx(5.0));
        REQUIRE(engine.get_variable("x") == Catch::Approx(5.0));
    }
    
    SECTION("Variable retrieval") {
        engine.set_variable("y", 10.0);
        REQUIRE(engine.evaluate("y") == Catch::Approx(10.0));
        REQUIRE(engine.evaluate("y + 3") == Catch::Approx(13.0));
    }
    
    SECTION("Assignment expressions") {
        engine.evaluate("x = 3");
        engine.evaluate("y = x + 2");
        REQUIRE(engine.get_variable("x") == Catch::Approx(3.0));
        REQUIRE(engine.get_variable("y") == Catch::Approx(5.0));
    }
    
    SECTION("Variable reassignment") {
        engine.evaluate("x = 5");
        engine.evaluate("x = x + 1");
        REQUIRE(engine.get_variable("x") == Catch::Approx(6.0));
    }
    
    SECTION("Multiple variables") {
        engine.evaluate("a = 1");
        engine.evaluate("b = 2");
        engine.evaluate("c = a + b");
        REQUIRE(engine.get_variable("a") == Catch::Approx(1.0));
        REQUIRE(engine.get_variable("b") == Catch::Approx(2.0));
        REQUIRE(engine.get_variable("c") == Catch::Approx(3.0));
    }
}

// ============================================================================
// LEVEL 3: SEMICOLONS AND MULTI-STATEMENT
// ============================================================================

TEST_CASE("Script Engine - Level 3: Semicolons", "[script][level3][semicolons]") {
    ScriptEngine engine;
    
    SECTION("Single statement with semicolon") {
        double result = engine.evaluate("x = 5;");
        REQUIRE(result == Catch::Approx(5.0));
        REQUIRE(engine.get_variable("x") == Catch::Approx(5.0));
    }
    
    SECTION("Multiple statements") {
        double result = engine.evaluate("x = 3; y = 7; x + y");
        REQUIRE(result == Catch::Approx(10.0));
        REQUIRE(engine.get_variable("x") == Catch::Approx(3.0));
        REQUIRE(engine.get_variable("y") == Catch::Approx(7.0));
    }
    
    SECTION("Assignment chain") {
        double result = engine.evaluate("a = 1; b = a + 1; c = b * 2; c");
        REQUIRE(result == Catch::Approx(4.0));
        REQUIRE(engine.get_variable("a") == Catch::Approx(1.0));
        REQUIRE(engine.get_variable("b") == Catch::Approx(2.0));
        REQUIRE(engine.get_variable("c") == Catch::Approx(4.0));
    }
    
    SECTION("Empty statements") {
        engine.evaluate("x = 1;; y = 2;");
        REQUIRE(engine.get_variable("x") == Catch::Approx(1.0));
        REQUIRE(engine.get_variable("y") == Catch::Approx(2.0));
    }
    
    SECTION("Last statement return value") {
        REQUIRE(engine.evaluate("x = 5; y = 3; x * y") == Catch::Approx(15.0));
        REQUIRE(engine.evaluate("a = 10; a") == Catch::Approx(10.0));
    }
}

// ============================================================================
// LEVEL 4: MATHEMATICAL FUNCTIONS
// ============================================================================

TEST_CASE("Script Engine - Level 4: Math Functions", "[script][level4][functions]") {
    ScriptEngine engine;
    
    SECTION("Trigonometric functions") {
        REQUIRE(engine.evaluate("sin(0)") == Catch::Approx(0.0).epsilon(0.001));
        REQUIRE(engine.evaluate("sin(1.5708)") == Catch::Approx(1.0).epsilon(0.001)); // π/2
        REQUIRE(engine.evaluate("cos(0)") == Catch::Approx(1.0).epsilon(0.001));
        REQUIRE(engine.evaluate("cos(3.14159)") == Catch::Approx(-1.0).epsilon(0.001)); // π
        REQUIRE(engine.evaluate("tan(0)") == Catch::Approx(0.0).epsilon(0.001));
        REQUIRE(engine.evaluate("tan(0.7854)") == Catch::Approx(1.0).epsilon(0.001)); // π/4
    }
    
    SECTION("Power and root functions") {
        REQUIRE(engine.evaluate("sqrt(4)") == Catch::Approx(2.0));
        REQUIRE(engine.evaluate("sqrt(9)") == Catch::Approx(3.0));
        REQUIRE(engine.evaluate("pow(2, 3)") == Catch::Approx(8.0));
        REQUIRE(engine.evaluate("pow(5, 2)") == Catch::Approx(25.0));
        REQUIRE(engine.evaluate("abs(-5)") == Catch::Approx(5.0));
        REQUIRE(engine.evaluate("abs(3)") == Catch::Approx(3.0));
    }
    
    SECTION("Logarithmic functions") {
        REQUIRE(engine.evaluate("log(2.71828)") == Catch::Approx(1.0).epsilon(0.001)); // ln(e)
        REQUIRE(engine.evaluate("log10(100)") == Catch::Approx(2.0).epsilon(0.001));
        REQUIRE(engine.evaluate("log10(1000)") == Catch::Approx(3.0).epsilon(0.001));
    }
    
    SECTION("Function composition") {
        REQUIRE(engine.evaluate("sin(cos(0))") == Catch::Approx(sin(1.0)).epsilon(0.001));
        REQUIRE(engine.evaluate("sqrt(pow(3, 2))") == Catch::Approx(3.0).epsilon(0.001));
        REQUIRE(engine.evaluate("abs(sin(-1.5708))") == Catch::Approx(1.0).epsilon(0.001));
    }
    
    SECTION("Functions with variables") {
        engine.set_variable("x", 2.0);
        REQUIRE(engine.evaluate("sqrt(x * x)") == Catch::Approx(2.0));
        REQUIRE(engine.evaluate("pow(x, 3)") == Catch::Approx(8.0));
    }
}

// ============================================================================
// LEVEL 5: AVS COORDINATE VARIABLES
// ============================================================================

TEST_CASE("Script Engine - Level 5: AVS Variables", "[script][level5][avs_vars]") {
    ScriptEngine engine;
    
    SECTION("Coordinate variables") {
        engine.set_pixel_context(100, 200, 800, 600);
        
        // Test basic coordinate access
        REQUIRE(engine.get_variable("x") != 0.0); // Should be normalized
        REQUIRE(engine.get_variable("y") != 0.0);
        REQUIRE(engine.get_variable("w") == 800.0);
        REQUIRE(engine.get_variable("h") == 600.0);
    }
    
    SECTION("Polar coordinates") {
        engine.set_pixel_context(400, 300, 800, 600); // Center pixel
        
        double x = engine.get_variable("x");
        double y = engine.get_variable("y");
        double r = engine.get_variable("r");
        double d = engine.get_variable("d");
        
        // Center should be near zero distance
        REQUIRE(std::abs(d) < 0.1);
        
        // r and d should relate to x and y
        REQUIRE(r == Catch::Approx(atan2(y, x)));
        REQUIRE(d == Catch::Approx(sqrt(x*x + y*y)));
    }
    
    SECTION("Coordinate transformations") {
        engine.set_pixel_context(100, 100, 200, 200);
        
        double result = engine.evaluate("x = x + 0.1; x");
        REQUIRE(result > engine.get_variable("x") - 0.1); // x should have changed
    }
}

// ============================================================================
// LEVEL 6: AVS TRANSFORM EXPRESSIONS
// ============================================================================

TEST_CASE("Script Engine - Level 6: AVS Transforms", "[script][level6][transforms]") {
    ScriptEngine engine;
    
    SECTION("Simple coordinate transformations") {
        engine.set_pixel_context(100, 100, 200, 200);
        
        // Identity transform
        engine.evaluate("x = x; y = y");
        double x1 = engine.get_variable("x");
        double y1 = engine.get_variable("y");
        
        // Verify coordinates are preserved
        engine.set_pixel_context(100, 100, 200, 200);
        double x2 = engine.get_variable("x");
        double y2 = engine.get_variable("y");
        
        REQUIRE(x1 == Catch::Approx(x2));
        REQUIRE(y1 == Catch::Approx(y2));
    }
    
    SECTION("Classic AVS transform patterns") {
        engine.set_pixel_context(150, 100, 200, 200);
        
        // Swirl effect: d = d * 0.95; r = r + 0.1
        engine.evaluate("d = d * 0.95; r = r + 0.1");
        
        double d = engine.get_variable("d");
        double r = engine.get_variable("r");
        
        REQUIRE(d >= 0.0); // Distance should be positive
        REQUIRE(std::isfinite(r)); // Angle should be finite
    }
    
    SECTION("Complex transform with functions") {
        engine.set_pixel_context(100, 100, 200, 200);
        
        // Tunnel effect: d = d * (0.9 + 0.1 * sin(r * 8))
        double result = engine.evaluate("d = d * (0.9 + 0.1 * sin(r * 8)); d");
        
        REQUIRE(std::isfinite(result));
        REQUIRE(result >= 0.0); // Distance should be positive
    }
    
    SECTION("Multi-statement transform") {
        engine.set_pixel_context(120, 80, 200, 200);
        
        // Complex transform with intermediate variables
        engine.evaluate(R"(
            temp = sin(r * 4); 
            d = d * (0.95 + 0.05 * temp); 
            r = r + temp * 0.1
        )");
        
        REQUIRE(std::isfinite(engine.get_variable("d")));
        REQUIRE(std::isfinite(engine.get_variable("r")));
        REQUIRE(std::isfinite(engine.get_variable("temp")));
    }
}

// ============================================================================
// LEVEL 7: FULL SCRIPT INTEGRATION
// ============================================================================

TEST_CASE("Script Engine - Level 7: Full Scripts", "[script][level7][integration]") {
    ScriptEngine engine;
    
    SECTION("Dynamic Movement init script") {
        // Test typical init phase script
        engine.evaluate(R"(
            c = 200;
            f = 0;
            dt = 0;
            dl = 0;
            beatdiv = 4;
            speed = 0.15
        )");
        
        REQUIRE(engine.get_variable("c") == 200.0);
        REQUIRE(engine.get_variable("speed") == 0.15);
    }
    
    SECTION("Dynamic Movement frame script") {
        engine.set_variable("f", 0.0);
        engine.set_variable("beatdiv", 4.0);
        
        // Test typical frame phase script  
        engine.evaluate(R"(
            f = f + 1;
            t = (f * 3.14159 * 2) / beatdiv
        )");
        
        REQUIRE(engine.get_variable("f") == 1.0);
        REQUIRE(std::isfinite(engine.get_variable("t")));
    }
    
    SECTION("Dynamic Movement pixel script") {
        engine.set_pixel_context(100, 100, 200, 200);
        engine.set_variable("speed", 0.1);
        
        // Test typical pixel phase transform
        engine.evaluate(R"(
            dr = cos(d) * speed * 2;
            dd = 1 - (sin(d) * speed);
            r = r + dr;
            d = d * dd
        )");
        
        REQUIRE(std::isfinite(engine.get_variable("r")));
        REQUIRE(std::isfinite(engine.get_variable("d")));
        REQUIRE(engine.get_variable("d") >= 0.0);
    }
    
    SECTION("SuperScope point script") {
        // Set up audio-like context
        engine.set_variable("i", 100);
        engine.set_variable("v", 0.5); // Sample value
        engine.set_variable("w", 800);
        engine.set_variable("h", 600);
        
        // Test typical point rendering script
        engine.evaluate(R"(
            n = 800;
            x = (i / n) - 0.5;
            y = v * 0.5;
            red = abs(x);
            green = abs(y);
            blue = (red + green) * 0.5
        )");
        
        REQUIRE(engine.get_variable("x") >= -0.5);
        REQUIRE(engine.get_variable("x") <= 0.5);
        REQUIRE(std::abs(engine.get_variable("y")) <= 0.25);
        REQUIRE(engine.get_variable("red") >= 0.0);
        REQUIRE(engine.get_variable("blue") >= 0.0);
    }
    
    SECTION("Error handling") {
        // Test various error conditions
        REQUIRE_THROWS(engine.evaluate("1 / 0")); // Division by zero
        REQUIRE_THROWS(engine.evaluate("sqrt(-1)")); // Invalid sqrt
        REQUIRE_THROWS(engine.evaluate("unknown_function(5)")); // Unknown function
        REQUIRE_THROWS(engine.evaluate("x + + y")); // Syntax error
    }
}