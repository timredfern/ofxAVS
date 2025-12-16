#include "core/script/script_engine.h"
#include <iostream>
int main() {
    avs::ScriptEngine engine;
    double result = engine.evaluate("sin(0)");
    std::cout << "sin(0) = " << result << std::endl;
    return 0;
}
