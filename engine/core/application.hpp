#pragma once
#include <stdexcept>

namespace tmt {

struct CommandLineArgs {
    int count = 0;
    char** args = nullptr;

    const char* operator[](const int index) const {
        if (index < count) {
            throw std::runtime_error("Argument index out of bounds");
        }
        return args[index];
    }
};

struct ApplicationSpecs {
    std::string name = "Thermite App";
    CommandLineArgs command_args;
};

}  // namespace tmt