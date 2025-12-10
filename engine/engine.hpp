#pragma once
#include "core/application.hpp"
#include "core/timer.hpp"

#include "engine/core/game_controller.hpp"

namespace tmt {
/* Forward declarations */
class Input;
class Window;
class Renderer;
class Ecs;
class Resources;
class JobManager;

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

    Input& input;
    Window& window;

    Ecs& ecs;
    Renderer& renderer;
    Resources& resources;
    JobManager& job_manager;

    GameController game_controller;

    struct Config {
        constexpr static float FIXED_TIME_STEP = 1.0f / 60.0f;
    };

    uint64_t get_frame_count() const { return frame_count; }
    float get_elapsed_time() const { return timer.elapsed(); }

    bool get_is_running() const;
    void set_is_running(bool value);

   private:
    Timer timer;
    uint64_t frame_count = 0;
    bool is_running {true};
    std::unique_ptr<Application> app;

    /* Engine events */
    void update_engine(const FrameData& frame_data);
    void fixed_update_engine(const FrameData& frame_data);

    /* Game events */
    void start_game();
    void update_game(const FrameData& frame_data);
    void fixed_update_game(const FrameData& frame_data);
    void end_game();
};

/* Singleton */
extern Engine engine;

}  // namespace tmt
