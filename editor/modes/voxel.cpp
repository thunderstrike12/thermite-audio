#include "voxel.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/resources/voxel_scene.hpp"
#include "engine/tools/svh_format.hpp"

#include "editor.hpp"
#include "editor/windows/node_hierarchy.hpp"

#include <imgui.h>

namespace tmt {

void VoxelMode::display_main_menu() {
    if (!ImGui::BeginMenu("File")) return;

    if (ImGui::MenuItem("New")) editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().new_svh();
    if (ImGui::MenuItem("Open...")) editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().open_svh();

    ImGui::Separator();

    if (ImGui::MenuItem("Save")) editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().save_svh();
    if (ImGui::MenuItem("Save As...")) editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().save_svh_as();

    ImGui::Separator();

    if (ImGui::BeginMenu("Import")) {
        if (ImGui::MenuItem("Vengi Voxel File (.vengi)")) editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().import_file("Vengi Voxel File", "vengi");
        if (ImGui::MenuItem("Sparse Voxel Hierarchy (.svh)")) editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().import_file("Sparse Voxel Hierarchy", "svh");

        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Export")) {
        if (ImGui::MenuItem("Wavefront (.obj)")) editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().export_file("Wavefront", "obj");

        ImGui::EndMenu();
    }

    ImGui::EndMenu();
}

void VoxelMode::on_switch_to(const std::any& meta_data) {
    engine.scenes.load_scene<VoxelEditScene>();

    if (meta_data.has_value()) {
        editor.switch_mode(Editor::Mode::VOXEL);
        editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().open_svh(std::any_cast<IO::FileLocation>(meta_data));

        engine.renderer.get_debug_transform().set_world_position(glm::vec3(0.0f, 0.0f, -32.0f));
        engine.renderer.get_debug_transform().set_world_rotation(glm::identity<glm::quat>());
    } else if (!edit_data.empty()) {
        std::vector<VoxelSceneNode> root_nodes = decode_svh(edit_data);

        // Use the old entity IDs for the voxel nodes to build the scene if there was a file open in the voxel editor previously, this is to avoid issues with the undo/redo system not finding
        // the correct entities.
        editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().build_scene(root_nodes, false, old_entity_mapping);
        old_entity_mapping.clear();

        engine.renderer.get_debug_camera() = cached_editor_camera;
        engine.renderer.get_debug_transform() = cached_editor_transform;
    }

    edit_data.clear();

    /* Switch to the albedo display mode, and disable TAA */
    engine.renderer.display_mode = DisplayMode::ALBEDO;
    engine.renderer.enable_taa = false;
}

void VoxelMode::on_switch_away() {
    NodeHierarchy& node_hierarchy = editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>();
    edit_data = node_hierarchy.encode_voxel_scene();

    // Save the old entity IDs of the nodes, this way we can restore them later and avoid issues with the undo/redo system not finding the right entities.
    const entt::basic_group node_group = engine.ecs.group<NodeHierarchy::NodeUUID>();
    for (const auto&& [entity, uuid] : node_group.each()) {
        old_entity_mapping.emplace(uuid.uuid, entity);
    }

    cached_editor_camera = engine.renderer.get_debug_camera();
    cached_editor_transform = engine.renderer.get_debug_transform();

    node_hierarchy.clear_root_entities();
    node_hierarchy.clear_selected_entities();

    /* Switch back to the default display mode, and re-enable TAA */
    engine.renderer.display_mode = DisplayMode::DEFAULT;
    engine.renderer.enable_taa = true;
}

}  // namespace tmt
