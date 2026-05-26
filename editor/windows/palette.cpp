#include "palette.hpp"

#include "editor.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/core/systems/undo_redo/type_diff.hpp"

#include <engine/engine.hpp>
#include <engine/core/ecs.hpp>
#include <engine/core/components/voxel_renderer.hpp>

#include "engine/shared/colorspace.hpp"

namespace tmt {

void Palette::set_selected_material_index(const MaterialIndex material_index) {
    if (selected_material_indices.size() == 1 && std::ranges::find(selected_material_indices, material_index) != selected_material_indices.end())
        return;  // Skip setting the material again and sending the diff to the undo redo system.

    TypeDiff diff { &selected_material_indices };

    diff.before();
    selected_material_indices.clear();
    selected_material_indices.push_back(material_index);
    update_material_editor = true;
    diff.after();

    // We set the value to its current value before and after (setting them both to true) because the value always has to be set to true when undo/redo is done (very hacky workaround).
    TypeDiff update_diff { &update_material_editor };
    update_diff.before();
    update_diff.after();

    UndoRedoCollection collection;
    collection.add_action(std::move(diff));
    collection.add_action(std::move(update_diff));
    UndoRedoCollection::send_to_manager(std::move(collection), "Select material");
}

void Palette::set_selected_material_indices(const MaterialIndex first, const MaterialIndex last) {
    TypeDiff diff { &selected_material_indices };

    diff.before();
    selected_material_indices.clear();
    for (uint32_t i = first; i <= last; i++) {
        selected_material_indices.push_back(static_cast<MaterialIndex>(i));
    }
    update_material_editor = true;
    diff.after();

    // We set the value to its current value before and after (setting them both to true) because the value always has to be set to true when undo/redo is done (very hacky workaround).
    TypeDiff update_diff { &update_material_editor };
    update_diff.before();
    update_diff.after();

    UndoRedoCollection collection;
    collection.add_action(std::move(diff));
    collection.add_action(std::move(update_diff));
    UndoRedoCollection::send_to_manager(std::move(collection), "Multi-select materials");
}

void Palette::toggle_selected_material_index(MaterialIndex material_index) {
    if (selected_material_indices.size() == 1 && selected_material_indices.front() == material_index) return;  // Avoid removing all selected items, should be at least 1.

    const auto selected_iterator = std::ranges::find(selected_material_indices, material_index);

    TypeDiff diff { &selected_material_indices };

    diff.before();
    if (selected_iterator != selected_material_indices.end()) {
        selected_material_indices.erase(selected_iterator);
    } else {
        selected_material_indices.push_back(material_index);
    }
    update_material_editor = true;
    diff.after();

    // We set the value to its current value before and after (setting them both to true) because the value always has to be set to true when undo/redo is done (very hacky workaround).
    TypeDiff update_diff { &update_material_editor };
    update_diff.before();
    update_diff.after();

    UndoRedoCollection collection;
    collection.add_action(std::move(diff));
    collection.add_action(std::move(update_diff));
    UndoRedoCollection::send_to_manager(std::move(collection), "Toggle material selection");
}

void Palette::before_begin() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
}

void Palette::end_display() {
    ImGui::PopStyleVar();
}

void Palette::on_inspect() {
    const auto selected_entity = editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().get_first_selected_entity();
    if (selected_entity == entt::null) return;

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;

    display_palette(renderer->resource);
}

void Palette::display_palette(const ResourceRef<VoxelVolume>& resource) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const ImVec2 frame_size = ImGui::GetContentRegionAvail() - Config::OUTSET * 2.0f;
    const ImVec2 cursor = ImGui::GetCursorScreenPos() + Config::OUTSET * 2.0f;

    /* Draw palette grid */
    const float entry_width = frame_size.x / Config::GRID_SIZE.x;
    const float entry_height = frame_size.y / Config::GRID_SIZE.y;
    for (uint32_t y = 0u; y < Config::GRID_SIZE.y; ++y) {
        for (uint32_t x = 0u; x < Config::GRID_SIZE.x; ++x) {
            const ImVec2 min = cursor + ImVec2(x * entry_width, y * entry_height);
            const ImVec2 max = cursor + ImVec2((x + 1u) * entry_width, (y + 1u) * entry_height);

            const uint32_t i = x + y * 8u;
            const Material& material = resource->blas->palette.entries[i];
            const glm::vec3 linear_srgb = cs::acescg_to_r709(material.albedo.unpack());
            glm::vec3 nonlinear_srgb = cs::delinearize(linear_srgb);

            const ImVec4 color = ImVec4(nonlinear_srgb.r, nonlinear_srgb.g, nonlinear_srgb.b, 1.0f);
            draw_list->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(color));
            draw_list->AddRect(min, max, ImColor(0xFF000000u), 0.0f, 0, 3.0f);

            ImGui::SetCursorScreenPos(min);
            const std::string button_id = "##palette_entry_" + std::to_string(i);
            if (ImGui::InvisibleButton(button_id.c_str(), ImVec2(entry_width, entry_height))) {
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
                    toggle_selected_material_index(static_cast<MaterialIndex>(i));
                } else if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
                    handle_multi_select(static_cast<MaterialIndex>(i));
                } else {
                    set_selected_material_index(static_cast<MaterialIndex>(i));
                }
            }
        }
    }

    /* Draw selected outlines */
    for (const MaterialIndex selected_material : selected_material_indices) {
        const uint32_t x = selected_material & 0b111u;
        const uint32_t y = selected_material >> 3;

        const ImVec2 min = cursor + ImVec2(x * entry_width, y * entry_height) - Config::OUTSET;
        const ImVec2 max = cursor + ImVec2((x + 1u) * entry_width, (y + 1u) * entry_height) + Config::OUTSET;

        draw_list->AddRect(min, max, ImColor(0xFF000000u), 0.0f, 0, 6.0f);
        draw_list->AddRect(min, max, ImColor(0xFFFFFFFFu), 0.0f, 0, 3.0f);
    }
}

void Palette::handle_multi_select(const MaterialIndex new_selection) {
    MaterialIndex smallest = selected_material_indices.front();

    for (const MaterialIndex material : selected_material_indices) {
        if (smallest <= material) continue;

        smallest = material;
    }

    if (smallest > new_selection) return;  // Return if the select value was lower than the lowest selected index.

    // Act as a regular select if the value select was the same as the lowest value.
    if (smallest == new_selection) {
        set_selected_material_index(smallest);
        return;
    }

    set_selected_material_indices(smallest, new_selection);
}

}  // namespace tmt