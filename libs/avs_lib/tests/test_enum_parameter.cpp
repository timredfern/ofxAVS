#include <catch2/catch_test_macros.hpp>
#include "core/parameter.h"
#include "core/coordinate_lookup_table.h"

using namespace avs;

TEST_CASE("Enum Parameter", "[parameter][enum]") {
    SECTION("Basic enum parameter creation") {
        std::vector<std::string> options = {"None", "Linear", "Nearest"};
        Parameter param("interpolation", ParameterType::ENUM, 0, options);
        
        REQUIRE(param.type() == ParameterType::ENUM);
        REQUIRE(param.as_int() == 0);
        REQUIRE(param.enum_value_name() == "None");
        REQUIRE(param.enum_options().size() == 3);
        REQUIRE(param.enum_options()[0] == "None");
        REQUIRE(param.enum_options()[1] == "Linear");
        REQUIRE(param.enum_options()[2] == "Nearest");
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
    
    SECTION("InterpolationMode enum usage") {
        // Test that InterpolationMode enum values match expected indices
        REQUIRE(static_cast<int>(InterpolationMode::NONE) == 0);
        REQUIRE(static_cast<int>(InterpolationMode::LINEAR) == 1);
        REQUIRE(static_cast<int>(InterpolationMode::NEAREST) == 2);
        
        std::vector<std::string> interp_options = {"None (Stepped)", "Linear", "Nearest"};
        Parameter param("interpolation", ParameterType::ENUM, 
                       static_cast<int>(InterpolationMode::LINEAR), interp_options);
        
        REQUIRE(param.as_int() == static_cast<int>(InterpolationMode::LINEAR));
        REQUIRE(param.enum_value_name() == "Linear");
    }
}