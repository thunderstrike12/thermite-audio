#pragma once
#include <stdexcept>
#include <string>
#include <filesystem>

#include "engine/events/game.hpp"

int main(int argc, char** argv);

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
    std::filesystem::path log_file;
};

class Application : public IGameEvents {
   public:
    Application(ApplicationSpecs specs) : specs(std::move(specs)) {};
    virtual ~Application() = default;

   private:
    const ApplicationSpecs specs;

    /* Befriend main to get specs */
    friend int ::main(int argc, char** argv);
};

}  // namespace tmt