#pragma once

namespace tmt {
class GameController {
   public:
    bool is_playing() const { return is_game_playing; }
    bool is_paused() const { return is_game_paused; }

    void start_game() { should_start_game = true; }
    void pause_game() { is_game_paused = true; }
    void resume_game() { is_game_paused = false; }
    void end_game() {
        should_end_game = true;
        is_game_paused = false;
    }

    bool should_game_start() const { return should_start_game; }
    bool should_game_end() const { return should_end_game; }

   private:
    friend class Engine;

    bool is_game_playing = false;
    bool is_game_paused = false;

#ifndef THERMITE_EDITOR
    bool should_start_game = true;
#else
    /* Will be set to false when we have an editor camera */
    bool should_start_game = true;
#endif
    bool should_end_game = false;
};
}  // namespace tmt