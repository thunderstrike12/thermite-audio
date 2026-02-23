#include "material_editor.hpp"

#include "editor.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/windows/palette.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <imgui.h>

namespace tmt {

void MaterialEditor::display() {
    const auto selected_entity = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().get_selected_entity();
    if (selected_entity == entt::null) {
        ImGui::TextWrapped("No voxel model selected.");
        return;
    }

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;

    const auto selected_material = editor.windows[Editor::Mode::VOXEL].get<Palette>().get_selected_material_index();
    Material& material = renderer->resource->blas->palette.entries[selected_material];

    ImGui::SeparatorText(("Material Index: " + std::to_string(selected_material)).c_str());

    ImGui::Separator();

    if (ImGui::ColorPicker3("Albedo Color", &material.albedo_r)) renderer->resource->set_dirty();
}

}  // namespace tmt