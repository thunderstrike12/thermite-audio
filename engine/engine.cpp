#include "engine.hpp"

#include <iostream>
#include <chrono>

#include "core/logger.hpp"
#include "core/input/input.hpp"

#include "core/window.hpp"
#include "core/audio.hpp"
#include "core/ecs.hpp"
#include "core/scenes.hpp"
#include "core/timer.hpp"
#include "core/polyline.hpp"

#include "core/renderer/renderer.hpp"

#include "systems/physics/physics_system.hpp"
#include "systems/animation/animation_system.hpp"
#include "systems/camera/camera_system.hpp"
#include "systems/ai/goap/goap_system.hpp"

#include "events/engine.hpp"
#include "events/game.hpp"
#include "events/scene.hpp"
#include "core/resources.hpp"
#include "core/salvo.hpp"
#include "core/input/input_map.hpp"
#include "tools/profiler.hpp"

bool tmt::Engine::get_is_running() const { return is_running; }
void tmt::Engine::set_is_running(bool value) { is_running = value; }

/* Singleton */
tmt::Engine tmt::engine;

namespace tmt {

Engine::Engine()
    : window(*new Window()),
      audio(*new Audio()),
      input_map(*new InputMap()),
      input(*new Input()),
      ecs(*new Ecs()),
      renderer(*new Renderer()),
      resources(*new Resources()),
      salvo(*new Salvo()),
      scenes(*new Scenes()),
      polyline(*new Polyline()) {}

Engine::~Engine() {
    /* Destruction should be in reverse order */
    delete &polyline;
    delete &scenes;
    delete &salvo;
    delete &resources;
    delete &renderer;
    delete &ecs;
    delete &input;
    delete &input_map;
    delete &audio;
    delete &window;
}

void Engine::init(std::unique_ptr<Application> user_app) {
    TMT_ZONE_SCOPED_N("Initializing")

    app = std::move(user_app);
    Log::init(app->specs.log_file.string());

    window.init(app->specs);
    input.init();
    renderer.init();
    audio.init();
    salvo.init();

    ecs.systems.add<Physics>();
    ecs.systems.add<RigModelManager>();
    ecs.systems.add<Goap>();

    OnEngineInit::dispatch(app->specs);
}

// Example stuff
void Engine::run() {
    timer.reset();

    float accumulator = 0.0f;
    while (is_running) {
        TMT_ZONE_SCOPED_N("Frame");

        if (game_controller.should_game_end()) {
            end_game();
            game_controller.should_end_game = false;
            game_controller.is_game_playing = false;
        }
        if (game_controller.should_game_start()) {
            start_game();
            game_controller.should_start_game = false;
            game_controller.is_game_playing = true;
        }
        if (game_controller.should_game_pause()) {
            pause_game();
            game_controller.is_game_paused = true;
            game_controller.should_pause_game = false;
        }
        if (game_controller.should_game_resume()) {
            resume_game();
            game_controller.is_game_paused = false;
            game_controller.should_resume_game = false;
        }

        const FrameData frame_data = {.delta_time = timer.tick()};

        input.update(frame_data);

        const bool should_update = game_controller.is_playing() && !game_controller.is_paused();
        /* Update */
        if (should_update) {
            update_game(frame_data);
        }
        update_engine(frame_data);

        audio.update();

        accumulator += frame_data.delta_time;
        while (accumulator >= Config::FIXED_TIME_STEP) {
            accumulator -= Config::FIXED_TIME_STEP;

            TMT_ZONE_SCOPED_N("Fixed Update")

            /* Fixed Update */
            if (should_update) {
                fixed_update_game(frame_data);
            }
            fixed_update_engine(frame_data);
        }

        OnEndFrame::dispatch();

        renderer.update();
        scenes.update();
        resources.unload_unused();
        frame_count++;
    }

    if (game_controller.is_playing()) {
        end_game();
    }
}

void Engine::end() {
    TMT_ZONE_SCOPED_N("Engine::end")

    OnEngineEnd::dispatch();

    salvo.end();
    ecs.clear();
    resources.unload_unused();
    renderer.end();
}

/* Engine events */
void Engine::update_engine(const FrameData& frame_data) {
    /* dispatch */
    TMT_ZONE_SCOPED_N("Engine::update_engine")

    OnEngineUpdate::dispatch(frame_data);
}
void Engine::fixed_update_engine(const FrameData& frame_data) {
    TMT_ZONE_SCOPED_N("Engine::fixed_update_engine")

    /* dispatch */
    OnEngineFixedUpdate::dispatch(frame_data);
}

/* Game events */
void Engine::start_game() {
    TMT_ZONE_SCOPED_N("Engine::start_game")

    app->on_start();
    OnGameStart::dispatch();
    if (scenes.get_active_scene()) scenes.get_active_scene()->on_start();
    OnSceneStart::dispatch();
}

void Engine::update_game(const FrameData& frame_data) {
    TMT_ZONE_SCOPED_N("Engine::update_game")

    app->on_update(frame_data);
    if (scenes.get_active_scene()) scenes.get_active_scene()->on_update(frame_data);
    OnGameUpdate::dispatch(frame_data);
}

void Engine::fixed_update_game(const FrameData& frame_data) {
    TMT_ZONE_SCOPED_N("Engine::fixed_update_game")
    app->on_fixed_update(frame_data);
    if (scenes.get_active_scene()) scenes.get_active_scene()->on_fixed_update(frame_data);
    OnGameFixedUpdate::dispatch(frame_data);
}

void Engine::pause_game() {
    TMT_ZONE_SCOPED_N("Engine::pause_game")
    OnGamePause::dispatch();
}

void Engine::resume_game() {
    TMT_ZONE_SCOPED_N("Engine::resume_game")
    OnGameResume::dispatch();
}

void Engine::end_game() {
    TMT_ZONE_SCOPED_N("Engine::end_game")
    app->on_end();
    OnGameEnd::dispatch();
    if (scenes.get_active_scene()) scenes.get_active_scene()->on_end();
    OnSceneEnd::dispatch();
}

}  // namespace tmt

#if THERMITE_DEBUG
#pragma message("THERMITE_DEBUG=1")
#else
#pragma message("THERMITE_DEBUG=0")
#endif
