#include "game_flow.hpp"
#include "imgui.h"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/tools/serializer/ecs.hpp"
#include "engine/core/input/input.hpp"

#include "editor/editor.hpp"
#include "editor/core//window.hpp"

#include "editor/windows/viewport.hpp"

namespace tmt {

void GameFlow::pause_game() {
    engine.game_controller.pause_game();
    was_mouse_locked = engine.input.is_mouse_locked();
    unlock_mouse();
}

void GameFlow::resume_game() {
    engine.game_controller.resume_game();
    if (was_mouse_locked) {
        lock_mouse();
    }
}

void GameFlow::start_game(const bool fullscreen_) {
    working_scene = engine.scenes.get_active_scene_type();
    cached_scene = Serializer::serialize(engine.ecs);
    fullscreen = fullscreen_;
    if (fullscreen_) {
        open_windows_before = editor.save_data.open_windows;

        for (auto& [name, open] : editor.save_data.open_windows) {
            if (name.find("Viewport") != std::string::npos) {
                open = true;
            } else {
                open = false;
            }
        }
    }

    engine.game_controller.start_game();
}

void GameFlow::end_game() {
    engine.game_controller.end_game();
    has_ended = true;
}

void GameFlow::on_editor_start() {}

void GameFlow::on_editor_update(const FrameData&) {
    /* Release Mouse */
    if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
        if (engine.input.is_mouse_locked()) {
            engine.input.set_game_preferred_mouse_lock(true);
            unlock_mouse();
        }
    }

    /* Ctrl + P to start / stop game */
    const bool ctrl_down = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    const bool p_down = ImGui::IsKeyPressed(ImGuiKey_P, false);
    const bool shift_down = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
    if (ctrl_down && p_down) {
        if (engine.game_controller.is_playing()) {
            if (shift_down) {
                if (engine.game_controller.is_paused()) {
                    resume_game();
                } else {
                    pause_game();
                }
            } else {
                end_game();
            }
        } else {
            start_game(shift_down);
        }
    }
}

void GameFlow::on_editor_end() {}

void GameFlow::unlock_mouse() {
    engine.input.lock_mouse(false);
    engine.input.set_mouse_relative_to_window(false);
}

void GameFlow::lock_mouse() {
    engine.input.lock_mouse(true);
    engine.input.set_mouse_relative_to_window(true);
}

void GameFlow::on_game_end() {
    unlock_mouse();
    if (fullscreen) {
        editor.save_data.open_windows = open_windows_before;
    }
    fullscreen = false;

    if (has_ended == false) return;
    engine.scenes.enqueue_scene(working_scene);
}

void GameFlow::on_pre_load_scene(PreLoadSceneEvent& event) {
    engine.input.set_game_preferred_mouse_lock(false);  // reset

    if (has_ended == false) return;
    event.scene_json = cached_scene;
    event.handled = true;
    has_ended = false;
}

}  // namespace tmt