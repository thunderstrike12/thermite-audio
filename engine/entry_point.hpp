#pragma once
#include "engine/core/application.hpp"
#include "engine/engine.hpp"

#if THERMITE_EDITOR
    #include "editor/editor.hpp"
#endif

extern std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args);

int main(int argc, char** argv) {
    tmt::engine.init(create_application({ argc, argv }));

#if THERMITE_EDITOR
    tmt::editor.init();
#endif

    tmt::engine.run();

    tmt::engine.end();
}