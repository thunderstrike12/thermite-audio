#pragma once
#include "engine/core/application.hpp"
#include "engine/engine.hpp"

extern std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args);

int main(int argc, char** argv) {
    {
        tmt::CommandLineArgs args {argc, argv};
        std::unique_ptr<tmt::Application> app = create_application(args);
        const tmt::ApplicationSpecs& specs = app->specs;
        tmt::engine.init(specs, std::move(app));
    }

    tmt::engine.run();

    tmt::engine.end();
}