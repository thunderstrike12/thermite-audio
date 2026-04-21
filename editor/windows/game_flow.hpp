#pragma once
#include "editor/core/window.hpp"
#include "engine/core/scenes.hpp"
#include "engine/events/game.hpp"
#include "engine/events/scene.hpp"

namespace tmt {

class GameFlow : public IEditorSystem<>, public OnGameEnd, public OnPreLoadScene {
   public:
    // Inherited via IWindow
    std::string get_title() const override { return ICON_MS_GAMEPAD "  Game Flow"; };

    void pause_game();

    void resume_game();

    void start_game(const bool fullscreen);

    void end_game();

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

    bool is_fullscreen() const { return fullscreen; }

   private:
    SceneIndex working_scene = NULL_SCENE;
    nlohmann::ordered_json cached_scene;
    bool has_ended = false;
    bool was_mouse_locked = false;

    bool fullscreen = false;
    std::unordered_map<std::string, bool> open_windows_before {};

    // Inherited via OnPreLoadScene
    void on_pre_load_scene(PreLoadSceneEvent& event) override;

    // Inherited via OnGameEnd
    void on_game_end() override;

    void unlock_mouse();
    void lock_mouse();
};

}  // namespace tmt