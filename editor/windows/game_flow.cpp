#include "game_flow.hpp"
#include "imgui.h"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/tools/serializer/ecs.hpp"

namespace tmt {

void GameFlow::display() {
    if (engine.game_controller.is_playing()) {
        if (ImGui::Button("End")) {
            engine.game_controller.end_game();
            has_ended = true;
        }
    } else {
        if (ImGui::Button("Start")) {
            working_scene = engine.scenes.get_active_scene_type();
            cached_scene = Serializer::serialize(engine.ecs);

            engine.game_controller.start_game();
        }
    }
    ImGui::SameLine();
    if (engine.game_controller.is_paused()) {
        if (ImGui::Button("Resume")) {
            engine.game_controller.resume_game();
        }
    } else {
        if (ImGui::Button("Pause")) {
            engine.game_controller.pause_game();
        }
    }
}

void GameFlow::on_editor_start() {}

void GameFlow::on_editor_update(const FrameData&) {}

void GameFlow::on_editor_end() {}

void GameFlow::on_game_end() {
    if (has_ended == false) return;
    engine.scenes.enqueue_scene(working_scene);
}

void GameFlow::on_pre_load_scene(PreLoadSceneEvent& event) {
    if (has_ended == false) return;
    event.scene_json = cached_scene;
    event.handled = true;
    has_ended = false;
}

}  // namespace tmt