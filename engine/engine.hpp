#pragma once

#include "core/application.hpp"

#include "engine/core/game_controller.hpp"

namespace tmt {

/* Forward declarations */
class InputMap;
class Input;
class Window;
class Renderer;
class Ecs;
class Resources;
class Audio;
class Salvo;
class Scenes;
class GameComponentRegistry;
class Polyline;
class PlayerData;

class Engine {
   public:
    Engine();
    ~Engine();

    /* No copies allowed */
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void init(std::unique_ptr<Application> user_app);
    void run();
    void end();

    /*
    How to declare a system:
    MySystem& my_system;

    Then in constructor:

    Engine() : my_system(*new MySystem()) {};

    **IMPORTANT**
    Also in the destructor

    ~Engine() { delete &my_system; };
    */

    Window& window;
    Audio& audio;

    InputMap& input_map;
    Input& input;
    Ecs& ecs;
    Renderer& renderer;
    Resources& resources;
    Salvo& salvo;
    Scenes& scenes;
    GameComponentRegistry& component_registry;
    Polyline& polyline;
    PlayerData& player_data;

    GameController game_controller;

    struct Config {
        constexpr static float FIXED_TIME_STEP = 1.0f / 60.0f;
        constexpr static uint32_t MAX_FIXED_UPDATES_PER_FRAME = 3;
    };

    const FrameData& frame_data() const;

    bool get_is_running() const;
    void set_is_running(bool value);

    const ApplicationSpecs& app_specs() const { return app->specs; }

   private:
    FrameData current_frame_data {};
    bool is_running { true };
    std::unique_ptr<Application> app;

    /* Engine events */
    void update_engine(const FrameData& frame_data);
    void fixed_update_engine(const FrameData& frame_data);

    /* Game events */
    void start_game();
    void update_game(const FrameData& frame_data);
    void fixed_update_game(const FrameData& frame_data);
    void pause_game();
    void resume_game();
    void end_game();

    void setup_signals();
    static void on_crash_signal(int signal);
};

/* Singleton */
extern Engine engine;

}  // namespace tmt
