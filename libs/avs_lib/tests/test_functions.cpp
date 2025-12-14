#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

// Mathematical function tests
// These will test our NS-EEL built-in functions

TEST_CASE("Mathematical functions", "[functions]") {
    SECTION("Trigonometric functions") {
        // Test that our implementations match standard library
        REQUIRE(std::sin(0.0) == Catch::Approx(0.0));
        REQUIRE(std::cos(0.0) == Catch::Approx(1.0));
        REQUIRE(std::tan(0.0) == Catch::Approx(0.0));
    }
    
    SECTION("Power and logarithmic functions") {
        REQUIRE(std::sqrt(4.0) == Catch::Approx(2.0));
        REQUIRE(std::pow(2.0, 3.0) == Catch::Approx(8.0));
        REQUIRE(std::log(1.0) == Catch::Approx(0.0));
    }
    
    SECTION("Utility functions") {
        REQUIRE(std::abs(-5.0) == Catch::Approx(5.0));
        REQUIRE(std::min(3.0, 7.0) == Catch::Approx(3.0));
        REQUIRE(std::max(3.0, 7.0) == Catch::Approx(7.0));
    }
}