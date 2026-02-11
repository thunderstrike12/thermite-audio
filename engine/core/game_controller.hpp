#pragma once

namespace tmt {

class GameController {
   public:
    bool is_playing() const { return is_game_playing; }
    bool is_paused() const { return is_game_paused; }
    bool is_running() const { return is_game_playing && !is_game_paused; }

    void start_game() { should_start_game = true; }
    void pause_game() { should_pause_game = true; }
    void resume_game() { should_resume_game = true; }
    void end_game() {
        should_end_game = true;
        is_game_paused = false;
    }

    bool should_game_start() const { return should_start_game; }
    bool should_game_pause() const { return should_pause_game; }
    bool should_game_resume() const { return should_resume_game; }
    bool should_game_end() const { return should_end_game; }

   private:
    friend class Engine;

    bool is_game_playing = false;
    bool is_game_paused = false;

#ifndef THERMITE_EDITOR
    bool should_start_game = true;
#else
    bool should_start_game = false;
#endif
    bool should_pause_game = false;
    bool should_resume_game = false;
    bool should_end_game = false;
};

}  // namespace tmt