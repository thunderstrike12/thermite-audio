#include "brush.hpp"

#include <engine/engine.hpp>
#include <engine/core/ecs.hpp>

#include "editor/imgui/types/all.hpp"
#include "editor.hpp"
#include "editor/windows/node_hierarchy.hpp"

#include <imgui.h>
#include <ImReflect_entry.hpp>

namespace tmt {

const std::unordered_map<Brush::Mode, const char*> Brush::MODE_ICONS {
    { Mode::MULTI_TOOL, ICON_MS_OPEN_WITH }, { Mode::PAINT, ICON_MS_BRUSH }, { Mode::COLOR_PICKER, ICON_MS_COLORIZE }, { Mode::ADD, ICON_MS_ADD_BOX }, { Mode::REMOVE, ICON_MS_DESTRUCTION },
};

void Brush::display() {
    for (const auto& [mode, icon] : MODE_ICONS) {
        // Keep everything on the same line (skip ImGui::SameLine on the first element).
        if (mode != MODE_ICONS.begin()->first) ImGui::SameLine();

        ImGui::BeginDisabled(active_mode == mode);

        if (ImGui::Button(icon)) active_mode = mode;

        ImGui::EndDisabled();
    }

    ImGui::SeparatorText("Settings");

    switch (active_mode) {
        case Mode::MULTI_TOOL: {
            const Entity selected_node = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().get_selected_entity();
            if (!engine.ecs.valid(selected_node)) break;

            const char* space_button_icon = (editor.gizmo.space ? ICON_MS_LANGUAGE : ICON_MS_VIEW_IN_AR);
            if (ImGui::Button(space_button_icon)) editor.gizmo.space = static_cast<uint8_t>(!editor.gizmo.space);

            ImGui::SameLine();

            // This button toggles between move and rotate, since scaling in the voxel editor isn't helpful.
            const char* mode_button_icon = Gizmo::gizmo_op_icons[editor.gizmo.operation];
            if (ImGui::Button(mode_button_icon)) editor.gizmo.operation = (editor.gizmo.operation + 1) % Gizmo::GIZMO_OP_COUNT;

            ImGui::NewLine();

            Transform& transform = engine.ecs.get_component<Transform>(selected_node);
            ImReflect::Input("", transform);

            break;
        }

        case Mode::PAINT:
        case Mode::COLOR_PICKER:
        case Mode::ADD:
        case Mode::REMOVE:
            break;
    }
}

}  // namespace tmt