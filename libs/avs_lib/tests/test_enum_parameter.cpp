#include <catch2/catch_test_macros.hpp>
#include "core/parameter.h"
#include "core/coordinate_grid.h"

using namespace avs;

TEST_CASE("Enum Parameter", "[parameter][enum]") {
    SECTION("Basic enum parameter creation") {
        std::vector<std::string> options = {"Rectangular", "Polar"};
        Parameter param("coord_mode", ParameterType::ENUM, 0, options);

        REQUIRE(param.type() == ParameterType::ENUM);
        REQUIRE(param.as_int() == 0);
        REQUIRE(param.enum_value_name() == "Rectangular");
        REQUIRE(param.enum_options().size() == 2);
        REQUIRE(param.enum_options()[0] == "Rectangular");
        REQUIRE(param.enum_options()[1] == "Polar");
    }

    SECTION("Enum parameter value changes") {
        std::vector<std::string> options = {"Option A", "Option B", "Option C"};
        Parameter param("test_enum", ParameterType::ENUM, 1, options);

        REQUIRE(param.as_int() == 1);
        REQUIRE(param.enum_value_name() == "Option B");

        // Change to option 2
        param.set_value(2);
        REQUIRE(param.as_int() == 2);
        REQUIRE(param.enum_value_name() == "Option C");

        // Change to option 0
        param.set_value(0);
        REQUIRE(param.as_int() == 0);
        REQUIRE(param.enum_value_name() == "Option A");
    }

    SECTION("Enum parameter bounds checking") {
        std::vector<std::string> options = {"Low", "Medium", "High"};
        Parameter param("quality", ParameterType::ENUM, 1, options);

        // Values should be clamped to valid range [0, 2]
        param.set_value(-1);
        REQUIRE(param.as_int() == 0);
        REQUIRE(param.enum_value_name() == "Low");

        param.set_value(5);
        REQUIRE(param.as_int() == 2);
        REQUIRE(param.enum_value_name() == "High");
    }

    SECTION("CoordMode enum usage") {
        // Test that CoordMode enum values match expected indices
        REQUIRE(static_cast<int>(CoordMode::RECTANGULAR) == 0);
        REQUIRE(static_cast<int>(CoordMode::POLAR) == 1);

        std::vector<std::string> mode_options = {"Rectangular", "Polar"};
        Parameter param("coord_mode", ParameterType::ENUM,
                       static_cast<int>(CoordMode::POLAR), mode_options);

        REQUIRE(param.as_int() == static_cast<int>(CoordMode::POLAR));
        REQUIRE(param.enum_value_name() == "Polar");
    }
}