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
    if (selected_material_index == material_index) return;  // Skip setting the material again and sending the diff to the undo redo system.

    TypeDiff diff { &selected_material_index };

    diff.before();
    selected_material_index = material_index;
    update_material_editor = true;
    diff.after();

    // We set the value to its current value before and after (setting them both to true) because the value always has to be set to true when undo/redo is done (very hacky workaround).
    TypeDiff update_diff { &update_material_editor };
    update_diff.before();
    update_diff.after();

    UndoRedoCollection collection;
    collection.add_action(std::move(diff));
    collection.add_action(std::move(update_diff));
    UndoRedoCollection::send_to_manager(std::move(collection), "Select Material");
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
                set_selected_material_index(static_cast<MaterialIndex>(i));
            }
        }
    }

    { /* Draw selected outline */
        const uint32_t x = selected_material_index & 0b111u;
        const uint32_t y = selected_material_index >> 3;

        const ImVec2 min = cursor + ImVec2(x * entry_width, y * entry_height) - Config::OUTSET;
        const ImVec2 max = cursor + ImVec2((x + 1u) * entry_width, (y + 1u) * entry_height) + Config::OUTSET;

        draw_list->AddRect(min, max, ImColor(0xFF000000u), 0.0f, 0, 6.0f);
        draw_list->AddRect(min, max, ImColor(0xFFFFFFFFu), 0.0f, 0, 3.0f);
    }
}

}  // namespace tmt