#include "scene.hpp"

#include "engine/engine.hpp"
#include "engine/core/scenes.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/tools/serializer/ecs.hpp"

#include <imgui.h>

#include "editor/core/systems/pop_up/pop_up.hpp"

namespace tmt {

SceneMode::SceneMode() : previous_scene_type(engine.scenes.get_active_scene_type()) {}

void SceneMode::display_main_menu() {
    if (ImGui::BeginMenu("Scene")) {
        const bool is_playing = engine.game_controller.is_playing();
        if (is_playing) ImGui::BeginDisabled();
        if (ImGui::MenuItem("Save Scene") && is_playing == false) {
            const bool succes = engine.scenes.serialize_active_scene();
            if (succes == false) {
                // clang-format off
                Notification::create()
                    .severity(Severity::ERROR)
                    .duration(5.0f)
                    .title("Failed to Save Scene")
                    .message("An error occurred while saving the scene. Check the logs for more details.");
                // clang-format on
            }
        }
        if (is_playing) ImGui::EndDisabled();
        ImGui::EndMenu();
    }
}

void SceneMode::on_switch_to(const std::any& /*meta_data*/) {
    is_switching_to = true;
    engine.scenes.load_scene(previous_scene_type);
    is_switching_to = false;

    cached_scene.clear();

    engine.renderer.get_debug_camera() = cached_editor_camera;
    engine.renderer.get_debug_transform() = cached_editor_transform;
}

void SceneMode::on_switch_away() {
    previous_scene_type = engine.scenes.get_active_scene_type();
    cached_scene = Serializer::serialize(engine.ecs);

    cached_editor_camera = engine.renderer.get_debug_camera();
    cached_editor_transform = engine.renderer.get_debug_transform();
}

void SceneMode::on_pre_load_scene(PreLoadSceneEvent& event) {
    if (!is_switching_to) return;

    // Use the cached json to load the object mode scene instead of the default one, this restores the previous working state.
    event.scene_json = std::move(cached_scene);
    event.handled = true;
}

}  // namespace tmt