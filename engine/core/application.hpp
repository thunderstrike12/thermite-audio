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
    const int test = 0;
};

class Application : public IGameEvents {
   public:
    Application(ApplicationSpecs specs) : specs(std::move(specs)) {};
    virtual ~Application() = default;

    const ApplicationSpecs specs;

    /* [Optional] */
    virtual void on_start() override {};
    virtual void on_update(const tmt::FrameData& time) override {};
    virtual void on_end() override {};
    virtual void on_fixed_update(const tmt::FrameData& time) override {};
};

}  // namespace tmt