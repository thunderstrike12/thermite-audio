#include <iostream>

#include "engine/engine.hpp"

int main(int argc, char** argv) {
#ifdef THERMITE_EDITOR
    std::cout << "Thermite Editor included.\n";
#else
    std::cout << "Thermite Editor not included.\n";
#endif

    tmt::engine.init();

    tmt::engine.run();

    tmt::engine.end();

    return 0;
}