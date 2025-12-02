#include "engine.hpp"

#include <iostream>
#include <chrono>

#include "core/logger.hpp"
#include "core/input.hpp"

#include "core/window.hpp"
#include "core/ecs.hpp"
#include "core/timer.hpp"

#include "core/renderer/renderer.hpp"

#include "systems/physics/physics_system.hpp"
#include "systems/animation/animation_system.hpp"

#include "events/engine.hpp"
#include "events/game.hpp"
#include "core/resources.hpp"

bool tmt::Engine::get_is_running() const { return is_running; }
void tmt::Engine::set_is_running(bool value) { is_running = value; }
/* Singleton */
tmt::Engine tmt::engine;

namespace tmt {

Engine::Engine() : input(*new Input()), window(*new Window()), ecs(*new Ecs()), renderer(*new Renderer()), resources(*new Resources()) {}

Engine::~Engine() {
    /* Destruction should be in reverse order */
    delete &resources;
    delete &renderer;
    delete &ecs;
    delete &window;
    delete &input;
}

void Engine::init(const ApplicationSpecs& specs) {
    Log::init(specs.log_file.string());

    input.init();
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
    while (is_running) {
        input.update();
        renderer.update();
        const FrameData frame_data = {.delta_time = timer.tick()};

        /* Update */
        OnUpdate::dispatch(frame_data);

        // Testing code here, please move when scenes can be added elegantly
        if (input.is_action_just_pressed("confirm")) {
            tmt::Log::info(tmt::Log::Scope::ENGINE, "Confirm action pressed!");
        }
        if (input.is_action_just_pressed("cancel")) {
            tmt::Log::info(tmt::Log::Scope::ENGINE, "Cancel action pressed!");
        }
        if (input.is_action_pressed("confirm")) {
            tmt::Log::info(tmt::Log::Scope::ENGINE, "Confirm pressed continuously!");
        }
        if (input.is_action_just_released("confirm")) {
            tmt::Log::info(tmt::Log::Scope::ENGINE, "Confirm released!");
        }

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
