#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/script/script_engine.h"

using namespace avs;

TEST_CASE("Basic arithmetic expressions", "[evaluator]") {
    ScriptEngine engine;
    
    SECTION("Simple addition") {
        double result = engine.evaluate("2 + 3");
        REQUIRE(result == Catch::Approx(5.0));
    }
    
    SECTION("Simple subtraction") {
        double result = engine.evaluate("10 - 4");
        REQUIRE(result == Catch::Approx(6.0));
    }
    
    SECTION("Simple multiplication") {
        double result = engine.evaluate("5 * 7");
        REQUIRE(result == Catch::Approx(35.0));
    }
    
    SECTION("Simple division") {
        double result = engine.evaluate("20 / 4");
        REQUIRE(result == Catch::Approx(5.0));
    }
}

TEST_CASE("Expression precedence", "[evaluator]") {
    ScriptEngine engine;
    
    SECTION("Multiplication before addition") {
        double result = engine.evaluate("2 + 3 * 4");
        REQUIRE(result == Catch::Approx(14.0)); // Should be 2 + 12, not 20
    }
    
    SECTION("Parentheses override precedence") {
        double result = engine.evaluate("(2 + 3) * 4");
        REQUIRE(result == Catch::Approx(20.0)); // Should be 5 * 4
    }
}

TEST_CASE("Variable support", "[evaluator]") {
    ScriptEngine engine;
    
    SECTION("Set and get variables") {
        engine.set_variable("x", 5.0);
        REQUIRE(engine.get_variable("x") == Catch::Approx(5.0));
    }
    
    SECTION("Use variables in expressions") {
        engine.set_variable("x", 3.0);
        engine.set_variable("y", 4.0);
        
        double result = engine.evaluate("x + y");
        REQUIRE(result == Catch::Approx(7.0));
    }
}