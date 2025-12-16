#include <catch2/catch_test_macros.hpp>
#include "core/script/script_engine.h"

using namespace avs;

TEST_CASE("Check semicolon support", "[semicolon]") {
    ScriptEngine engine;
    
    SECTION("Single expression with semicolon") {
        double result = engine.evaluate("x = 5;");
        REQUIRE(result == 5.0);
    }
    
    SECTION("Multi-statement expression") {
        double result = engine.evaluate("x = 3; y = 7; x + y");
        REQUIRE(result == 10.0);
        REQUIRE(engine.get_variable("x") == 3.0);
        REQUIRE(engine.get_variable("y") == 7.0);
    }
    
    SECTION("Empty statements") {
        double result = engine.evaluate("x = 1;; y = 2;");
        REQUIRE(engine.get_variable("x") == 1.0);
        REQUIRE(engine.get_variable("y") == 2.0);
    }
}