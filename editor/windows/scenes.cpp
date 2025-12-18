#include "scenes.hpp"

#include <imgui.h>

#include "engine/engine.hpp"
#include "engine/core/scenes.hpp"

namespace tmt {

void ScenesWindow::display() {
    const auto& scenes = engine.scenes.get_registered_scenes();
    const auto& active_scene = engine.scenes.get_active_scene();
    const auto& scene_name = active_scene ? active_scene->get_name() : "None";
    if (ImGui::BeginCombo("Scenes", scene_name.data())) {
        for (const auto& [type_index, scene_info] : scenes) {
            bool is_selected = (engine.scenes.get_active_scene() && typeid(*engine.scenes.get_active_scene()) == type_index);
            if (ImGui::Selectable(scene_info.name.c_str(), is_selected)) {
                engine.scenes.enqueue_scene(type_index);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

void ScenesWindow::on_editor_start() {}

void ScenesWindow::on_editor_update(const FrameData& time) {}

void ScenesWindow::on_editor_end() {}

}  // namespace tmt