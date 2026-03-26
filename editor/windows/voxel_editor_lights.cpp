#include "voxel_editor_lights.hpp"

#include "editor/imgui/components/all.hpp"

#include "engine/core/ecs.hpp"
#include "engine/core/components/light.hpp"
#include "engine/core/renderer/renderer.hpp"

#include <imgui.h>

namespace tmt {

void VoxelEditorLights::on_editor_update(const FrameData&) {
    const entt::basic_view lights = engine.ecs.view<Light>();

    const Transform& light_transform = engine.ecs.get_component<Transform>(lights.front());
    const Entity camera_follower = light_transform.get_parent();

    Transform& follower_transform = engine.ecs.get_component<Transform>(camera_follower);

    const glm::mat4& matrix = engine.renderer.get_debug_transform().get_world_matrix();
    follower_transform.set_world_matrix(matrix);
}

void VoxelEditorLights::on_inspect() {
    const entt::basic_view lights = engine.ecs.view<Light>();

    auto&& [transform, light] = engine.ecs.get_component<Transform, Light>(lights.front());
    ImReflect::Input("", transform);
    ImReflect::Input("", light);

    // Sketchy dummy to avoid an imgui crash to do with the cursor.
    const float item_spacing_y = ImGui::GetStyle().ItemSpacing.y;
    ImGui::Dummy(ImVec2(0.0f, item_spacing_y * 0.5f));
}

}  // namespace tmt