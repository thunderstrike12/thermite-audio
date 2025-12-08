#include "hierarchy.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/core/components/name.hpp"
#include "engine/core/components/transform.hpp"

namespace tmt {

void Hierarchy::start_section() {
    /* Begin child section for hierarchy */
    ImGui::BeginChild("HierarchySection", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
}

void Hierarchy::end_section() {
    ImGui::EndChild();

    drag_drop_target(entt::null);

    if (end_above_next_frame != NULL_INDEX) {
        previous_end_above = selected_index_end_above;
        selected_index_end_above = end_above_next_frame;
        end_above_next_frame = NULL_INDEX;
    } else {
        previous_end_above = NULL_INDEX;
    }

    if (end_below_next_frame != NULL_INDEX) {
        previous_end_below = selected_index_end_below;
        selected_index_end_below = end_below_next_frame;
        end_below_next_frame = NULL_INDEX;
    } else {
        previous_end_below = NULL_INDEX;
    }
}

bool Hierarchy::display_entity(const HierarchyState& state) {
    const bool root_indent = state.depth() == 0;
    const bool has_parent = state.transform.has_parent();
    if (root_indent && has_parent) return false;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    flags |= ImGuiTreeNodeFlags_FramePadding;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;

    const bool is_selected = selected_entities.contains(state.entity);
    if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

    const bool is_leaf = state.transform.has_children() == false;
    if (is_leaf) flags |= ImGuiTreeNodeFlags_Leaf;

    const Entity parent = state.transform.get_parent();
    const bool is_begin = state.position == selected_index_begin && selection_parent == parent;
    if (is_begin) flags |= ImGuiTreeNodeFlags_Bullet;

    /* Debug name */
    // const auto name = state.name.name + " " + std::to_string(state.position.x) + "-" + std::to_string(state.position.y);
    const auto name = state.name.name;

    const ImGuiID tree_node_id = ImGui::GetID(name.c_str());
    bool open_node = ImGui::TreeNodeBehavior(tree_node_id, flags, name.c_str(), NULL);
    /* TODO: is a too early because it toggles when mouse down, but we use mouse released for selection */

    const bool is_hovered = ImGui::IsItemHovered();
    const bool is_left_mouse_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    const bool is_payload_being_dragged = ImGui::GetDragDropPayload() != nullptr;
    const bool is_left_clicked = is_hovered && is_left_mouse_released;
    const bool is_ctrl_held = ImGui::GetIO().KeyCtrl;
    const bool is_shift_held = ImGui::GetIO().KeyShift;
    const bool same_parent = selection_parent == parent;

    /* Checks if hovering the arrow part of the tree node */
    const bool hover_arrow = is_hovered && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) <= ImGui::GetTreeNodeToLabelSpacing();

    /* Drag and drop logic */
    drag_drop_source(state.entity);
    const bool dropped = drag_drop_target(state.entity);
    /* Force open treenode using imgui storage */
    if (dropped) {
        ImGuiStorage* storage = ImGui::GetStateStorage();
        storage->SetBool(tree_node_id, true);
    }

    /* Selection logic */
    const bool select_entity = !dropped && !hover_arrow && !is_payload_being_dragged && is_left_clicked;
    if (select_entity) {
        if (is_shift_held && same_parent) {
            const bool is_first_selected = selected_entities.empty();
            if (is_first_selected) {
                selected_index_begin = state.position;
                selection_parent = parent;
                selected_index_end_above = state.position;
                selected_index_end_below = state.position;
            } else {
                if (state.position.y > selected_index_begin.y) {
                    end_below_next_frame = state.position;
                } else {
                    end_above_next_frame = state.position;
                }
            }
        } else if (is_ctrl_held) {
            if (is_selected) {
                selected_entities.erase(state.entity);
            } else {
                selected_entities.insert(state.entity);
            }
        } else {
            selected_entities.clear();
            selected_entities.insert(state.entity);
            selected_index_begin = state.position;
            selection_parent = state.transform.get_parent();
            selected_index_end_above = state.position;
            selected_index_end_below = state.position;
        }
    }

    /* Check if entity is between selected */
    const bool is_between_begin_above = Hierarchy::is_between(state.position, selected_index_end_above, selected_index_begin);
    const bool is_between_begin_below = Hierarchy::is_between(state.position, selected_index_begin, selected_index_end_below);
    const bool is_between = same_parent && (is_between_begin_above || is_between_begin_below);
    if (is_between) {
        selected_entities.insert(state.entity);
    }

    const bool was_between_begin_above = Hierarchy::is_between(state.position, previous_end_above, selected_index_begin);
    const bool was_between_begin_below = Hierarchy::is_between(state.position, selected_index_begin, previous_end_below);
    const bool was_between = was_between_begin_above || was_between_begin_below;
    if (!is_between && was_between) {
        selected_entities.erase(state.entity);
    }

    /* If the node is closed, don't display children */
    if (open_node == false) return true;

    const bool has_children = state.transform.has_children();
    /* Display children */
    if (has_children) {
        uint32_t level_index = state.index() + 1;
        for (auto child : state.transform.get_children()) {
            const bool has_components = engine.ecs.has_component<Name, Transform>(child);
            if (!has_components) continue;

            auto& child_transform = engine.ecs.get_component<Transform>(child);
            auto& child_name = engine.ecs.get_component<Name>(child);

            HierarchyState child_state(child, child_transform, child_name);
            child_state.position.x = state.depth() + 1;
            child_state.position.y = level_index;

            const bool displayed = display_entity(child_state);
            if (displayed) level_index++;
        }
    }

    ImGui::TreePop();
    return true;
}

bool Hierarchy::drag_drop_source(const Entity dragged_entity) const {
    if (ImGui::BeginDragDropSource()) {
        DragNDropPayload payload_data;
        const bool multiple = selected_entities.size() > 1 && selected_entities.contains(dragged_entity);
        payload_data.multiple = multiple;
        payload_data.entity = dragged_entity;

        ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &payload_data, sizeof(DragNDropPayload));
        std::string name;
        if (multiple) {
            name = engine.ecs.get_component<Name>(dragged_entity).name;
            const int count = static_cast<int>(selected_entities.size());
            name += " (+" + std::to_string(count - 1) + " more)";
        } else {
            name = engine.ecs.get_component<Name>(dragged_entity).name;
        }
        ImGui::TextUnformatted(name.c_str());
        ImGui::EndDragDropSource();
        return true;
    }
    return false;
}

bool Hierarchy::drag_drop_target(const Entity dropped_entity) {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
            const DragNDropPayload* payload_data = (DragNDropPayload*)payload->Data;
            if (payload_data->multiple) {
                /* Multiple entities being moved */
                for (const auto& selected_entity : selected_entities) {
                    if (selected_entity == dropped_entity) continue;
                    auto& transform = engine.ecs.get_component<Transform>(selected_entity);
                    transform.set_parent(dropped_entity);
                }
            } else {
                /* Single entity being moved */
                Entity single_entity = payload_data->entity;
                if (dropped_entity != single_entity) {
                    auto& transform = engine.ecs.get_component<Transform>(single_entity);
                    transform.set_parent(dropped_entity);
                }
            }
            ImGui::EndDragDropTarget();
            selected_index_begin = NULL_INDEX;
            selection_parent = entt::null;
            selected_index_end_above = NULL_INDEX;
            selected_index_end_below = NULL_INDEX;
            return true;
        }
        ImGui::EndDragDropTarget();
    }
    return false;
}

}  // namespace tmt