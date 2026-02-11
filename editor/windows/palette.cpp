#include "palette.hpp"

#include "editor.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "engine/core/components/voxel_renderer.hpp"

namespace tmt {

void Palette::before_begin() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
}

void Palette::end_display() {
    ImGui::PopStyleVar();
}

void Palette::display() {
    const auto selected_entity = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().get_selected_entity();
    if (selected_entity == entt::null) return;

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;

    display_palette(renderer->resource);

    if (ImGui::Begin(ICON_MS_EDIT " Material Editor")) {
        display_material_editor();
    }
    ImGui::End();
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

            const ImVec4 color = ImVec4(material.albedo_r, material.albedo_g, material.albedo_b, 1.0f);
            draw_list->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(color));
            draw_list->AddRect(min, max, ImColor(0xFF000000u), 0.0f, 0, 3.0f);

            ImGui::SetCursorScreenPos(min);
            const std::string button_id = "##palette_entry_" + std::to_string(i);
            if (ImGui::InvisibleButton(button_id.c_str(), ImVec2(entry_width, entry_height))) {
                selected_material_index = static_cast<MaterialIndex>(i);
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

void Palette::display_material_editor() const {
    /* Doesn't actually work yet but for the idea */
    const auto selected_entity = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().get_selected_entity();
    if (selected_entity == entt::null) {
        ImGui::TextWrapped("No voxel model selected.");
        return;
    }

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;

    Material& material = renderer->resource->blas->palette.entries[selected_material_index];
    ImGui::Text("Editing Material Index: %u", selected_material_index);
    ImGui::Separator();
    if (ImGui::ColorPicker3("Albedo Color", &material.albedo_r)) renderer->resource->set_dirty();
}

}  // namespace tmt