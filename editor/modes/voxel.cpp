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

    if (ImGui::MenuItem("New")) editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().new_svh();
    if (ImGui::MenuItem("Open...")) editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().open_svh();

    ImGui::Separator();

    if (ImGui::MenuItem("Save")) editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().save_svh();
    if (ImGui::MenuItem("Save As...")) editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().save_svh_as();

    ImGui::Separator();

    if (ImGui::BeginMenu("Import")) {
        if (ImGui::MenuItem("Vengi Voxel File (.vengi)")) editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().import_file("Vengi Voxel File", "vengi");
        if (ImGui::MenuItem("Sparse Voxel Hierarchy (.svh)")) editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().import_file("Sparse Voxel Hierarchy", "svh");

        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Export")) {
        if (ImGui::MenuItem("Wavefront (.obj)")) editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().export_file("Wavefront", "obj");

        ImGui::EndMenu();
    }

    ImGui::EndMenu();
}

void VoxelMode::on_switch_to(const std::any& meta_data) {
    engine.scenes.load_scene<VoxelEditScene>();

    if (meta_data.has_value()) {
        editor.switch_mode(Editor::Mode::VOXEL);
        editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().open_svh(std::any_cast<IO::FileLocation>(meta_data));

        engine.renderer.get_debug_transform().set_world_position(glm::vec3(0.0f, 0.0f, -32.0f));
        engine.renderer.get_debug_transform().set_world_rotation(glm::identity<glm::quat>());
    } else if (!edit_data.empty()) {
        std::vector<VoxelSceneNode> root_nodes = decode_svh(edit_data);

        editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().build_scene(root_nodes);

        engine.renderer.get_debug_camera() = cached_editor_camera;
        engine.renderer.get_debug_transform() = cached_editor_transform;
    }

    edit_data.clear();

    /* Switch to the albedo display mode, and cache the previous display mode */
    cached_display_mode = engine.renderer.display_mode;
    engine.renderer.display_mode = DisplayMode::ALBEDO;
}

void VoxelMode::on_switch_away() {
    NodeHierarchy& node_hierarchy = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>();
    edit_data = node_hierarchy.encode_voxel_scene();

    cached_editor_camera = engine.renderer.get_debug_camera();
    cached_editor_transform = engine.renderer.get_debug_transform();

    /* Switch back to the previously cached display mode */
    engine.renderer.display_mode = cached_display_mode;

    node_hierarchy.clear_root_entities();
}

}  // namespace tmt
