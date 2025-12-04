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
#include "systems/camera/camera_system.hpp"

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

void Engine::init(const ApplicationSpecs& specs, std::unique_ptr<Application> user_app) {
    app = std::move(user_app);
    Log::init(specs.log_file.string());

    input.init();
    window.init(specs);
    renderer.init();

    ecs.register_system<Physics>();
    ecs.register_system<Animation>();
    ecs.register_system<CameraSystem>();

    OnEngineInit::dispatch(specs);
}

// Example stuff
void Engine::run() {
    timer.reset();
    start_game();

    float accumulator = 0.0f;
    while (is_running) {
        input.update();

        const FrameData frame_data = {.delta_time = timer.tick()};

        /* Update */
        update_game(frame_data);
        update_engine(frame_data);

        accumulator += frame_data.delta_time;
        while (accumulator >= Config::FIXED_TIME_STEP) {
            accumulator -= Config::FIXED_TIME_STEP;

            /* Fixed Update */
            fixed_update_game(frame_data);
            fixed_update_engine(frame_data);
        }

        OnEndFrame::dispatch();

        renderer.update();
        frame_count++;
    }
    end_game();
}

void Engine::end() {
    OnEngineEnd::dispatch();
    renderer.end();
}

/* Engine events */
void Engine::update_engine(const FrameData& frame_data) {
    /* dispatch */
    OnEngineUpdate::dispatch(frame_data);
}
void Engine::fixed_update_engine(const FrameData& frame_data) {
    /* dispatch */
    OnEngineFixedUpdate::dispatch(frame_data);
}

/* Game events */
void Engine::start_game() {
    app->on_start();
    OnGameStart::dispatch();
}

void Engine::update_game(const FrameData& frame_data) {
    app->on_update(frame_data);
    OnGameUpdate::dispatch(frame_data);
}

void Engine::fixed_update_game(const FrameData& frame_data) {
    app->on_fixed_update(frame_data);
    OnGameFixedUpdate::dispatch(frame_data);
}

void Engine::end_game() {
    app->on_end();
    OnGameEnd::dispatch();
}

}  // namespace tmt

#if THERMITE_DEBUG
#pragma message("THERMITE_DEBUG=1")
#else
#pragma message("THERMITE_DEBUG=0")
#endif