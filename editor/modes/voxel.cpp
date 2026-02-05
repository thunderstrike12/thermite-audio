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

void VoxelMode::on_switch_to() {
    engine.scenes.load_scene<VoxelEditScene>();

    if (!edit_data.empty()) {
        std::vector<VoxelSceneNode> root_nodes = decode_svh(edit_data);

        editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().build_scene(root_nodes);
    }

    edit_data.clear();

    engine.renderer.get_debug_camera() = cached_editor_camera;
    engine.renderer.get_debug_transform() = cached_editor_transform;
}

void VoxelMode::on_switch_away() {
    edit_data = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().encode_voxel_scene();

    cached_editor_camera = engine.renderer.get_debug_camera();
    cached_editor_transform = engine.renderer.get_debug_transform();
}

}  // namespace tmt