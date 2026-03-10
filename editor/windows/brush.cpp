#include "brush.hpp"

#include <engine/engine.hpp>
#include <engine/core/ecs.hpp>
#include <engine/tools/serializer/all.hpp>

#include "editor.hpp"
#include "editor/imgui/extra.hpp"
#include "editor/imgui/types/all.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/core/systems/undo_redo/component_diff.hpp"

#include <imgui.h>
#include <ImReflect_entry.hpp>

namespace tmt {

namespace {

struct ButtonInfo {
    const char* icon;
    std::string_view title;
    std::string_view desc;
};

const std::vector<std::pair<Brush::Tool, ButtonInfo>> TOOL_INFOS {
    { Brush::Tool::GIZMO, { ICON_MS_OPEN_WITH, "Selection Gizmo", "Select, translate, rotate, and scale objects." } },
    { Brush::Tool::COLOR_PICKER, { ICON_MS_COLORIZE, "Eye Dropper", "Select the palette entry of a voxel." } },
    { Brush::Tool::SINGLE, { ICON_MS_DEPLOYED_CODE, "Modify Single Voxel", "Add, remove, or paint individual voxels." } },
    { Brush::Tool::BOX, { ICON_MS_GRID_ON, "Modify Voxel Box", "Add, remove, or paint a 3D area (box) of voxels." } },
};

const std::vector<std::pair<Brush::Mode, ButtonInfo>> MODE_INFOS {
    { Brush::Mode::ATTACH, { ICON_MS_ADD, "Add", "Add (or replace) voxels." } },
    { Brush::Mode::REMOVE, { ICON_MS_REMOVE, "Remove", "Remove voxels." } },
    { Brush::Mode::PAINT, { ICON_MS_BRUSH, "Paint", "Set palette entry of voxels." } },
};

}  // namespace

void Brush::display() {
    for (const auto [tool, info] : TOOL_INFOS) {
        // Keep everything on the same line (skip ImGui::SameLine on the first element).
        if (tool != TOOL_INFOS.begin()->first) ImGui::SameLine();

        ImGui::BeginDisabled(state.tool == tool);

        if (ImGui::Button(info.icon)) state.tool = tool;
        tooltip(info.title.data(), info.desc.data());

        ImGui::EndDisabled();
    }

    ImGui::SeparatorText("Settings");

    switch (state.tool) {
        case Tool::GIZMO: {
            const Entity selected_node = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().get_first_selected_entity();
            if (!engine.ecs.valid(selected_node)) break;

            const auto&& [name, transform] = engine.ecs.get_component<Name, Transform>(selected_node);

            // Edit the name of the node.
            ImResponse name_response = ImReflect::Input("Name", name);
            const auto name_type_response = name_response.get<Name>();

            static std::unique_ptr<ComponentDiff<Name>> name_diff;
            if (name_type_response.is_activated()) {
                name_diff = std::make_unique<ComponentDiff<Name>>(selected_node);
                name_diff->before();
            } else if (name_type_response.is_deactivated_after_edit()) {
                name_diff->after();
                name_diff->commit();
            }

            ImGui::NewLine();

            // Switch between local and world space gizmo.
            const char* space_button_icon = (editor.gizmo.space ? ICON_MS_LANGUAGE : ICON_MS_VIEW_IN_AR);
            if (ImGui::Button(space_button_icon)) editor.gizmo.space = static_cast<uint8_t>(!editor.gizmo.space);

            ImGui::SameLine();

            // Switch between gizmo operations (translation, rotation, scale).
            const char* mode_button_icon = Gizmo::gizmo_op_icons[editor.gizmo.operation];
            if (ImGui::Button(mode_button_icon)) editor.gizmo.operation = (editor.gizmo.operation + 1) % Gizmo::GIZMO_OP_COUNT;

            ImGui::SameLine();

            // Switch between modes of multi object transformations.
            const char* multi_button_icon = editor.gizmo.multiselect_mode ? ICON_MS_FILTER_NONE : ICON_MS_FILTER_1;
            if (ImGui::Button(multi_button_icon)) editor.gizmo.multiselect_mode = static_cast<uint8_t>(!editor.gizmo.multiselect_mode);

            // Handle undo/redo of the transform using the component diff.
            ImResponse transform_response = ImReflect::Input("", transform);
            const auto transform_type_response = transform_response.get<Transform>();

            static std::unique_ptr<ComponentDiff<Transform>> transform_diff;
            if (transform_type_response.is_activated()) {
                transform_diff = std::make_unique<ComponentDiff<Transform>>(selected_node);
                transform_diff->before();
            } else if (transform_type_response.is_deactivated_after_edit()) {
                transform_diff->after();
                transform_diff->commit();
            }
            break;
        }

        case Tool::COLOR_PICKER:
            break;

        case Tool::SINGLE:
        case Tool::BOX:
            const float width = ImGui::GetContentRegionAvail().x;
            const float button_width = (width - ImGui::GetStyle().FramePadding.x * 2.0f) / 3.0f;

            for (const auto [mode, info] : MODE_INFOS) {
                // Keep everything on the same line (skip ImGui::SameLine on the first element).
                if (mode != MODE_INFOS.begin()->first) {
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetItemRectMax().x);
                }

                ImGui::BeginDisabled(state.mode == mode);
                if (ImGui::Button(info.icon, ImVec2 { button_width, 0.0f })) state.mode = mode;
                tooltip(info.title.data(), info.desc.data());
                ImGui::EndDisabled();
            }

            break;
    }
}

}  // namespace tmt