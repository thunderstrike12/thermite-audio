#include "game_flow.hpp"
#include "imgui.h"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

namespace tmt {

void GameFlow::display() {
    if (engine.game_controller.is_playing()) {
        if (ImGui::Button("End")) {
            engine.game_controller.end_game();
            engine.ecs.clear();
        }
    } else {
        if (ImGui::Button("Start")) {
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

}  // namespace tmt