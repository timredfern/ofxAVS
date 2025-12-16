#include <catch2/catch_test_macros.hpp>
#include "core/script/script_engine.h"
using namespace avs;
TEST_CASE("Debug function test") {
    ScriptEngine engine;
    double result = engine.evaluate("sin(0)");
    printf("sin(0) = %f
", result);
    result = engine.evaluate("2 + 3");  
    printf("2 + 3 = %f
", result);
}
