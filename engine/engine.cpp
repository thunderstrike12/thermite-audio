#include "engine.hpp"

#include <iostream>
#include <chrono>

#include "core/logger.hpp"
#include "core/window.hpp"
#include "core/ecs.hpp"
#include "core/timer.hpp"

#include "core/renderer/renderer.hpp"

#include "systems/physics/physics_system.hpp"
#include "systems/animation/animation_system.hpp"

#include "events/engine.hpp"
#include "events/game.hpp"
#include "core/resources.hpp"

/* Singleton */
tmt::Engine tmt::engine;

namespace tmt {

Engine::Engine() : window(*new Window()), ecs(*new Ecs()), renderer(*new Renderer()), resources(*new Resources()) {}

Engine::~Engine() {
    /* Destruction should be in reverse order */
    delete &resources;
    delete &renderer;
    delete &ecs;
    delete &window;
}

void Engine::init(const ApplicationSpecs& specs) {
    Log::init(specs.log_file.string());
    tmt::Log::info("No scope here!");
    tmt::Log::warn(Log::Scope::GAME, "Game scope here!");
    tmt::Log::warn(Log::Scope::RENDERER, "Renderer scope here!");
    tmt::Log::warn(Log::Scope::ENGINE, "THERMITE scope here!");

    window.init(specs);
    renderer.init();

    ecs.register_system<Physics>();
    ecs.register_system<Animation>();

    OnEngineInit::dispatch(specs);
}

// Example stuff
void Engine::run() {
    timer.reset();
    OnStart::dispatch();

    float accumulator = 0.0f;
    while (window.is_running) {
        window.update();
        renderer.update();
        const FrameData frame_data = {.delta_time = timer.tick()};

        /* Update */
        OnUpdate::dispatch(frame_data);

        accumulator += frame_data.delta_time;
        while (accumulator >= Config::FIXED_TIME_STEP) {
            accumulator -= Config::FIXED_TIME_STEP;

            /* Fixed Update */
            OnFixedUpdate::dispatch(frame_data);
        }

        OnEndFrame::dispatch();
        frame_count++;
    }

    OnEnd::dispatch();
}

void Engine::end() { OnEngineEnd::dispatch(); }

}  // namespace tmt
