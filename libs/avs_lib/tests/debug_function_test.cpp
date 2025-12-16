#include <catch2/catch_test_macros.hpp>
#include "core/script/script_engine.h"
#include <iostream>

using namespace avs;

TEST_CASE("Debug function parsing", "[debug]") {
    ScriptEngine engine;
    
    SECTION("Simple number") {
        double result = engine.evaluate("42");
        std::cout << "42 = " << result << std::endl;
        REQUIRE(result == 42.0);
    }
    
    SECTION("Simple addition") {
        double result = engine.evaluate("2 + 3");
        std::cout << "2 + 3 = " << result << std::endl;
        REQUIRE(result == 5.0);
    }
    
    SECTION("sin function") {
        double result = engine.evaluate("sin(0)");
        std::cout << "sin(0) = " << result << std::endl;
        // Just check if it runs, don't require correctness yet
    }
    
    SECTION("Check if sin is being parsed as function") {
        // This will help us see if the parser recognizes sin as a function
        try {
            double result = engine.evaluate("sin");
            std::cout << "sin (variable) = " << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "sin as variable failed: " << e.what() << std::endl;
        }
    }
}