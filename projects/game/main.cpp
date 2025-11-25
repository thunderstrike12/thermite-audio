#include <iostream>

#include "engine/engine.hpp"

int main(int argc, char** argv) {
#ifdef THERMITE_EDITOR
    std::cout << "Thermite Editor included.\n";
#else
    std::cout << "Thermite Editor not included.\n";
#endif

    tmt::ApplicationSpecs specs {.name = "Game", .command_args = {argc, argv}};

    tmt::engine.init(specs);

    tmt::engine.run();

    tmt::engine.end();

    return 0;
}