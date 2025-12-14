#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "script/script_engine.h"
#include <cmath>

using namespace avs;

TEST_CASE("Mathematical functions", "[functions]") {
    ScriptEngine engine;
    
    SECTION("Trigonometric functions") {
        SECTION("sin function") {
            double result = engine.evaluate("sin(0)");
            REQUIRE(result == Catch::Approx(0.0));
            
            result = engine.evaluate("sin(1.5708)");  // π/2
            REQUIRE(result == Catch::Approx(1.0).epsilon(0.001));
        }
        
        SECTION("cos function") {
            double result = engine.evaluate("cos(0)");
            REQUIRE(result == Catch::Approx(1.0));
            
            result = engine.evaluate("cos(3.14159)");  // π
            REQUIRE(result == Catch::Approx(-1.0).epsilon(0.001));
        }
        
        SECTION("tan function") {
            double result = engine.evaluate("tan(0)");
            REQUIRE(result == Catch::Approx(0.0));
            
            result = engine.evaluate("tan(0.7854)");  // π/4
            REQUIRE(result == Catch::Approx(1.0).epsilon(0.001));
        }
    }
    
    SECTION("Power and root functions") {
        SECTION("sqrt function") {
            double result = engine.evaluate("sqrt(4)");
            REQUIRE(result == Catch::Approx(2.0));
            
            result = engine.evaluate("sqrt(9)");
            REQUIRE(result == Catch::Approx(3.0));
            
            result = engine.evaluate("sqrt(2)");
            REQUIRE(result == Catch::Approx(1.414).epsilon(0.01));
        }
        
        SECTION("pow function") {
            double result = engine.evaluate("pow(2, 3)");
            REQUIRE(result == Catch::Approx(8.0));
            
            result = engine.evaluate("pow(5, 2)");
            REQUIRE(result == Catch::Approx(25.0));
            
            result = engine.evaluate("pow(10, 0)");
            REQUIRE(result == Catch::Approx(1.0));
        }
        
        SECTION("abs function") {
            double result = engine.evaluate("abs(-5)");
            REQUIRE(result == Catch::Approx(5.0));
            
            result = engine.evaluate("abs(3.14)");
            REQUIRE(result == Catch::Approx(3.14));
        }
    }
    
    SECTION("Logarithmic functions") {
        SECTION("log function (natural log)") {
            double result = engine.evaluate("log(1)");
            REQUIRE(result == Catch::Approx(0.0));
            
            result = engine.evaluate("log(2.71828)");  // e
            REQUIRE(result == Catch::Approx(1.0).epsilon(0.001));
        }
        
        SECTION("log10 function") {
            double result = engine.evaluate("log10(1)");
            REQUIRE(result == Catch::Approx(0.0));
            
            result = engine.evaluate("log10(100)");
            REQUIRE(result == Catch::Approx(2.0));
        }
    }
    
    SECTION("Function composition") {
        SECTION("Functions in expressions") {
            double result = engine.evaluate("sin(0) + cos(0)");
            REQUIRE(result == Catch::Approx(1.0));
            
            result = engine.evaluate("sqrt(4) * 3");
            REQUIRE(result == Catch::Approx(6.0));
            
            result = engine.evaluate("pow(sqrt(9), 2)");
            REQUIRE(result == Catch::Approx(9.0));
        }
        
        SECTION("Nested functions") {
            double result = engine.evaluate("sqrt(pow(3, 2))");
            REQUIRE(result == Catch::Approx(3.0));
            
            result = engine.evaluate("sin(cos(0))");
            REQUIRE(result == Catch::Approx(std::sin(1.0)).epsilon(0.001));
        }
    }
}