#include "hierarchy.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include <ImReflect.hpp>

#include "editor/editor.hpp"
#include "editor/windows/viewport.hpp"

#include "engine/engine.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/scene_view.hpp"

#include "engine/core/components/name.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include "engine/tools/serializer/all.hpp"
#include "engine/tools/prefab_helper.hpp"

#include "editor/events/scene.hpp"

#include "editor/shared/colors.hpp"
#include "editor/shared/icons.hpp"
#include "editor/shared/theme.hpp"

#include "editor/core/systems/undo_redo/entity_diff.hpp"
#include "editor/core/systems/undo_redo/component_diff.hpp"
#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"

namespace tmt {

/* Strips any trailing " (N)" suffix from a name to get the base name. */
static std::string get_base_name(const std::string& name) {
    if (name.size() < 4) return name;

    // Check if the name ends with " (N)" where N is one or more digits
    if (name.back() != ')') return name;

    auto paren_open = name.rfind(" (");
    if (paren_open == std::string::npos) return name;

    // Check that everything between " (" and ")" is digits
    const std::string between = name.substr(paren_open + 2, name.size() - paren_open - 3);
    if (between.empty()) return name;

    for (char c : between) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return name;
    }

    return name.substr(0, paren_open);
}

/* Finds the next available duplicate number for a base name, considering all existing entity names. */
static int find_next_duplicate_number(const std::string& base_name, const Registry& registry) {
    int max_number = 0;

    auto view = registry.view<Name>();
    for (auto [entity, name_component] : view.each()) {
        const std::string& existing_name = name_component.name;

        if (existing_name == base_name) {
            // The base name itself exists, so we need at least (1)
            max_number = std::max(max_number, 1);
        } else if (existing_name.size() > base_name.size()) {
            // Check if it matches "base_name (N)"
            const std::string prefix = base_name + " (";
            if (existing_name.rfind(prefix, 0) == 0 && existing_name.back() == ')') {
                const std::string num_str = existing_name.substr(prefix.size(), existing_name.size() - prefix.size() - 1);
                bool all_digits = !num_str.empty();
                for (char c : num_str) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) {
                        all_digits = false;
                        break;
                    }
                }
                if (all_digits) {
                    int num = std::stoi(num_str);
                    max_number = std::max(max_number, num + 1);
                }
            }
        }
    }

    return max_number == 0 ? 1 : max_number;
}

void Hierarchy::before_begin() {
    /* zero margin */
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    const bool prefab_mode = editor.editor_mode == Editor::Mode::PREFAB;
    if (prefab_mode) {
        /* nice blue-ish background color */
        ImGui::PushStyleColor(ImGuiCol_WindowBg, colors::to_u32(colors::PREFAB_BACKGROUND));
    }
}

void Hierarchy::on_inspect() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    top_bar();
    render_hierarchy();
    context_menu(entt::null);

    file_drag_drop(entt::null);

    ImGui::PopStyleVar();
}

void Hierarchy::end_display() {
    const bool prefab_mode = editor.editor_mode == Editor::Mode::PREFAB;
    if (prefab_mode) {
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();
}

void Hierarchy::start_section() {
    /* Begin child section for hierarchy */
    ImGui::BeginChild("HierarchySection", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
}

void Hierarchy::end_section() {
    ImGui::EndChild();

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        clear_selection();
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered()) {
        ImGui::OpenPopup(Config::RIGHT_CLICK_CONTEXT);
    }

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

void Hierarchy::context_menu(const Entity hovered_entity) {
    const ImGuiID prefab_popup_id = ImGui::GetID(Config::PREFAB_POP_UP);

    if (ImGui::BeginPopup(Config::RIGHT_CLICK_CONTEXT)) {
        if (selected_entities.empty() == false) { /* Delete selection */
            const auto text = selected_entities.size() > 1 ? "Delete Entities" : "Delete Entity";
            if (ImGui::MenuItem(text)) {
                delete_selection();
                ImGui::CloseCurrentPopup();
            }
        }
        if (selected_entities.empty() == false) { /* Copy selection */
            const auto text = selected_entities.size() > 1 ? "Copy Entities" : "Copy Entity";
            if (ImGui::MenuItem(text)) {
                copy_selection();
                ImGui::CloseCurrentPopup();
            }
        }
        { /* Paste selection */
            const auto text = "Paste Entities";
            if (ImGui::MenuItem(text)) {
                paste_entities(hovered_entity);
                ImGui::CloseCurrentPopup();
            }
        }
        const bool is_playing = engine.game_controller.is_playing();
        if (selected_entities.size() == 1 && is_playing == false) { /* Make Prefab */
            ImGui::Separator();
            const auto text = "Make Prefab...";
            if (ImGui::MenuItem(text)) {
                ImGui::OpenPopup(prefab_popup_id);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(Config::PREFAB_POP_UP, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
        static std::string prefab_name = "NewPrefab";
        static std::string error_message = "";
        ImGui::InputText("Prefab Name", &prefab_name, ImGuiInputTextFlags_AlwaysOverwrite);
        ImGui::SameLine();
        ImGui::Text(".prefab");
        if (ImGui::Button("Create")) {
            const IO::FileLocation temp_location = { IO::Location::PROJECT, prefab_name + ".prefab" };
            if (IO::file_exists(temp_location) == false) {
                const Entity root_entity = *EntityHelper::upper_parents(selected_entities).begin();
                const bool has_prefab = engine.ecs.has_component<Prefab>(root_entity);
                if (has_prefab == false) {
                    PrefabHelper::create_prefab(temp_location, root_entity);
                    prefab_name = "NewPrefab";
                    ImGui::CloseCurrentPopup();
                    OnSceneModified::dispatch();
                } else {
                    error_message = "Selected entity is already part of a prefab, cannot create prefab from it.";
                }
            } else {
                error_message = "A prefab with the name \"" + prefab_name + "\" already exists.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            prefab_name = "NewPrefab";
            ImGui::CloseCurrentPopup();
        }

        if (error_message.empty() == false) {
            ImGui::TextColored(colors::to_imvec4(colors::ERROR), "%s", error_message.c_str());
        }

        ImGui::EndPopup();
    }
}

void Hierarchy::copy_selection() {
    const auto json = Serializer::serialize(std::set(selected_entities.begin(), selected_entities.end()), tmt::engine.ecs);
    ImGui::SetClipboardText(json.dump().c_str());
}

std::set<Entity> Hierarchy::paste_entities(const Entity hovered_entity, const bool overwrite_parent) {
    const char* clipboard_text = ImGui::GetClipboardText();
    if (clipboard_text == nullptr) {
        Log::warn("Failed to paste entities, clipboard is empty.");
        return {};
    }
    const json deserialized = json::parse(clipboard_text, nullptr, false);
    if (deserialized.is_discarded()) {
        Log::error("Failed to parse clipboard JSON for entities. Clipboard is: \"{}\"", clipboard_text);
        return {};
    }

    std::set<Entity> new_entities;
    Serializer::deserialize(deserialized, new_entities, tmt::engine.ecs);

    const std::set<Entity> parents = EntityHelper::upper_parents(new_entities);
    if (overwrite_parent) {
        for (const Entity parent : parents) {
            Transform& parent_transform = tmt::engine.ecs.get_component<Transform>(parent);
            parent_transform.set_parent(hovered_entity);
        }
    }

    EntityDiff diff { parents, true };
    EntityDiff::send_to_manager(std::move(diff), "Pasted Entities");

    clear_selection();
    for (const auto entity : parents) {
        selected_entities.insert(entity);
    }
    OnSceneModified::dispatch();
    return new_entities;
}

void Hierarchy::duplicate_selection() {
    if (selected_entities.empty()) return;

    copy_selection();
    const std::set<Entity> new_entities = paste_entities(entt::null, false);

    // Rename duplicated entities with incrementing " (N)" suffixes
    auto& reg = tmt::engine.ecs.get_registry();
    for (const Entity entity : new_entities) {
        if (!reg.all_of<Name>(entity)) continue;

        Name& name_comp = reg.get<Name>(entity);
        const std::string base = get_base_name(name_comp.name);
        const int next_number = find_next_duplicate_number(base, reg);
        name_comp.name = base + " (" + std::to_string(next_number) + ")";
    }
}

void Hierarchy::delete_selection() {
    if (selected_entities.empty()) return;

    for (const auto selected : selected_entities) {
        const bool is_prefab = engine.ecs.has_component<Prefab>(selected);
        if (is_prefab == false) continue;

        const auto& prefab = engine.ecs.get_component<Prefab>(selected);
        const auto& transform = engine.ecs.get_component<Transform>(selected);
        auto parents = transform.get_all_parents();
        for (const auto parent : parents) {
            if (parent == selected) continue;
            if (selected_entities.contains(parent)) continue;
            if (engine.ecs.has_component<Prefab>(parent) == false) continue;
            const auto& parent_prefab = engine.ecs.get_component<Prefab>(parent);
            if (parent_prefab.instance_id == prefab.instance_id) {
                Log::warn("Cannot delete selection because it contains a child prefab.", selected, parent);
                return;
            }
        }
    }

    const std::set<Entity> upper_parents = EntityHelper::upper_parents(selected_entities);
    EntityDiff diff { upper_parents, false };
    EntityDiff::send_to_manager(std::move(diff), "Deleted Entities");

    for (const auto selected : selected_entities) {
        tmt::engine.ecs.destroy_entity(selected);
    }
    clear_selection();
    OnSceneModified::dispatch();
}

void Hierarchy::clear_selection() {
    selected_entities.clear();
    selection_parent = entt::null;

    selected_index_begin = NULL_INDEX;
    selected_index_end_above = NULL_INDEX;
    selected_index_end_below = NULL_INDEX;
    end_above_next_frame = NULL_INDEX;
    end_below_next_frame = NULL_INDEX;
    previous_end_above = NULL_INDEX;
    previous_end_below = NULL_INDEX;
}

int Hierarchy::get_window_flags() const {
    return ImGuiWindowFlags_MenuBar;
}

void Hierarchy::on_editor_update(const FrameData&) {
    for (int i = 0; i < selected_entities.size(); ++i) {
        if (!engine.ecs.valid(selected_entities[i])) continue;
        VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entities[i]);
        if (renderer) renderer->outlined = true;
    }

    for (int i = 0; i < selected_entities.size(); ++i) {
        const auto entity = *std::next(selected_entities.begin(), i);
        if (engine.ecs.valid(entity) == false) {
            selected_entities.erase(entity);
            break;
        }
    }

    const bool is_playing = engine.game_controller.is_running();
    const bool using_debug_cam = editor.systems[editor.editor_mode].get<Viewport>().is_using_debug_camera();
    const bool wants_keyboard = ImGui::GetIO().WantCaptureKeyboard;
    const bool has_selection = !selected_entities.empty();

    /* print bools */
    if (!is_playing && !using_debug_cam && !wants_keyboard) {
        const bool ctrl_held = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
        if (ctrl_held) {
            if (has_selection && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
                copy_selection();
            } else if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
                paste_entities(entt::null, false);
            } else if (has_selection && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
                duplicate_selection();
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            delete_selection();
        }
    }
}

void Hierarchy::file_drag_drop(const tmt::Entity parent) {
    if (ImGui::BeginDragDropTarget()) {
        // Get the current payload to check if its a FileLocation.
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload != nullptr && payload->IsDataType("FileLocation")) {
            const std::string_view json_string { static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize) };
            tmt::IO::FileLocation file_location;
            tmt::Serializer::deserialize(nlohmann::ordered_json::parse(json_string), file_location);

            const std::string& extension = file_location.relative_path.extension().generic_string();
            if (extension == PrefabHelper::Config::PREFAB_EXTENSION && ImGui::AcceptDragDropPayload("FileLocation") != nullptr) {
                const Entity root = PrefabHelper::instantiate_prefab(file_location, parent);
                clear_selection();
                selected_entities.insert(root);
                OnSceneModified::dispatch();
            } else if (VoxelScene::SUPPORTED_FILE_EXTENSIONS.contains(extension) && ImGui::AcceptDragDropPayload("FileLocation") != nullptr) {
                const ResourceRef<VoxelScene>& scene = engine.resources.load_resource<VoxelScene>(file_location);
                const std::vector<Entity> root_entities = scene->instantiate_entities();

                for (const Entity root_entity : root_entities) {
                    Transform& transform = engine.ecs.get_component<Transform>(root_entity);
                    transform.set_parent(parent);
                }
                OnSceneModified::dispatch();
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void Hierarchy::top_bar() {
    /*menu bar*/
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Empty Entity")) {
                auto entity = engine.ecs.create_entity();
                engine.ecs.add_or_get_component<Transform>(entity);
                clear_selection();
                selected_entities.insert(entity);

                EntityDiff diff { entity, true };
                EntityDiff::send_to_manager(std::move(diff), "Created Entity");
            }
            ImGui::EndMenu();
        }

        ImGui::InputTextWithHint("##EntitySearch", ICON_MS_SEARCH " Search entities...", &filter);

        ImGui::EndMenuBar();
    }
}

bool Hierarchy::display_entity(const HierarchyState& state, uint32_t& row) {
    const auto scope_id = ImReflect::Detail::scope_id((int)state.entity);

    const bool disabled = engine.ecs.is_disabled(state.entity);

    static float base_alpha = ImGui::GetStyle().Alpha;
    const float alpha = disabled ? base_alpha * ImGui::GetStyle().DisabledAlpha : base_alpha;
    const auto scope_disabled = ImReflect::Detail::scope_style(ImGuiStyleVar_Alpha, alpha);

    const bool filtering = filter.empty() == false;
    if (filtering) {
        /* Lower case everything */
        std::string name_lower = state.name.name;
        std::string filter_lower = filter;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);
        if (name_lower.find(filter_lower) == std::string::npos) {
            return false;
        }
    } else {
        const bool root_indent = state.depth() == 0;
        const bool has_parent = state.transform.has_parent();
        if (root_indent && has_parent) return false;
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    flags |= ImGuiTreeNodeFlags_FramePadding;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;
    flags |= ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;

    const bool is_selected = selected_entities.contains(state.entity);
    if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

    const bool is_leaf = state.transform.has_children() == false;
    if (is_leaf || filtering) flags |= ImGuiTreeNodeFlags_Leaf;

    const Entity parent = state.transform.get_parent();
    // const bool is_begin = state.position == selected_index_begin && selection_parent == parent;
    // if (is_begin) flags |= ImGuiTreeNodeFlags_Bullet;

    /* Debug name */
    // const auto name = state.name.name + " " + std::to_string(state.position.x) + "-" + std::to_string(state.position.y);
    auto name = state.name.name.empty() ? "Entity_" + EntityHelper::to_string(state.entity) : state.name.name;

    const bool is_prefab = engine.ecs.has_component<Prefab>(state.entity);
    if (is_prefab) {
        name = tmt::icons::PREFAB_ICON + name;
    }

    const ImGuiID tree_node_id = ImGui::GetID(name.c_str());

    /* Draw alternating row background for odd rows */
    if (row % 2 != 0) {
        const ImVec2 row_min = ImVec2(ImGui::GetWindowPos().x, ImGui::GetCursorScreenPos().y);
        const ImVec2 row_max = ImVec2(row_min.x + ImGui::GetWindowWidth(), row_min.y + ImGui::GetFrameHeight());
        const ImU32 alt_row_color = ImGui::ColorConvertFloat4ToU32(tmt::theme::TABLE_ROW_BG_ALT);
        ImGui::GetWindowDrawList()->AddRectFilled(row_min, row_max, alt_row_color);
    }
    row++;

    if (is_prefab) ImGui::PushStyleColor(ImGuiCol_Text, tmt::colors::to_u32(tmt::colors::PREFAB));
    bool open_node = ImGui::TreeNodeBehavior(tree_node_id, flags, name.c_str(), NULL);
    if (is_prefab) ImGui::PopStyleColor();

    const bool is_hovered = ImGui::IsItemHovered();
    const bool is_left_mouse_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    const bool is_right_mouse_released = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
    const bool is_payload_being_dragged = ImGui::GetDragDropPayload() != nullptr;
    const bool is_left_clicked = is_hovered && is_left_mouse_released;
    const bool is_right_clicked = is_hovered && is_right_mouse_released;
    const bool is_ctrl_held = ImGui::GetIO().KeyCtrl;
    const bool is_shift_held = ImGui::GetIO().KeyShift;
    const bool same_parent = selection_parent == parent;

    /* Checks if hovering the arrow part of the tree node */
    const bool hover_arrow = is_hovered && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) <= ImGui::GetTreeNodeToLabelSpacing();

    /* Drag and drop logic */
    drag_drop_source(state.entity);
    const bool dropped = drag_drop_target(state.entity);
    /* Force open treenode using imgui storage */
    if (const bool contains = force_open_entities.contains(state.entity) || dropped) {
        ImGuiStorage* storage = ImGui::GetStateStorage();
        storage->SetBool(tree_node_id, true);

        if (contains) force_open_entities.erase(state.entity);
    }

    /* Selection logic */
    const bool select_entity = !dropped && !hover_arrow && !is_payload_being_dragged && (is_left_clicked || is_right_clicked);
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
        } else if (is_right_clicked) {
            const bool already_selected = selected_entities.contains(state.entity);
            if (already_selected == false) {
                selected_entities.clear();
                selected_entities.insert(state.entity);
                selected_index_begin = state.position;
                selection_parent = state.transform.get_parent();
                selected_index_end_above = state.position;
                selected_index_end_below = state.position;
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

    if (is_right_clicked) {
        ImGui::OpenPopup(Config::RIGHT_CLICK_CONTEXT);
    }
    context_menu(state.entity);
    file_drag_drop(state.entity);

    /* If the node is closed, don't display children */
    if (open_node == false) return true;
    if (filtering) {
        if (open_node) ImGui::TreePop();
        return true;
    }

    const bool has_children = state.transform.has_children();
    /* Display children */
    if (has_children) {
        uint32_t level_index = state.index() + 1;
        /* copy since the list can change while we loop */
        auto children = state.transform.get_children();
        for (const auto child : children) {
            const bool has_components = engine.ecs.has_component<Name, Transform>(child);
            if (!has_components) continue;

            auto& child_transform = engine.ecs.get_component<Transform>(child);
            auto& child_name = engine.ecs.get_component<Name>(child);

            HierarchyState child_state(child, child_transform, child_name);
            child_state.position.x = state.depth() + 1;
            child_state.position.y = level_index;

            const bool displayed = display_entity(child_state, row);
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

            /* Collect the entities that will be moved */
            std::vector<Entity> entities_to_move;
            if (payload_data->multiple) {
                for (const auto& selected_entity : selected_entities) {
                    if (selected_entity == dropped_entity) continue;
                    entities_to_move.push_back(selected_entity);
                }
            } else {
                if (dropped_entity != payload_data->entity) {
                    entities_to_move.push_back(payload_data->entity);
                }
            }

            /* Collect all unique affected parents (old parents + new parent) for a single before/after snapshot each */
            std::map<Entity, ComponentDiff<Transform>> parent_diffs;

            if (dropped_entity != entt::null && engine.ecs.valid(dropped_entity)) {
                parent_diffs.emplace(dropped_entity, ComponentDiff<Transform> { dropped_entity });
            }

            for (const Entity entity : entities_to_move) {
                const Entity old_parent = engine.ecs.get_component<Transform>(entity).get_parent();
                if (old_parent != entt::null && engine.ecs.valid(old_parent) && !parent_diffs.contains(old_parent)) {
                    parent_diffs.emplace(old_parent, ComponentDiff<Transform> { old_parent });
                }
            }

            /* Snapshot all parents and children BEFORE mutations */
            for (auto& [_, diff] : parent_diffs) diff.before();

            std::vector<ComponentDiff<Transform>> child_diffs;
            child_diffs.reserve(entities_to_move.size());
            for (const Entity entity : entities_to_move) {
                child_diffs.emplace_back(entity);
                child_diffs.back().before();
            }

            /* Perform all re-parenting */
            for (const Entity entity : entities_to_move) {
                auto& transform = engine.ecs.get_component<Transform>(entity);
                transform.set_parent(dropped_entity);
            }

            /* Snapshot all parents and children AFTER mutations */
            for (auto& [_, diff] : parent_diffs) diff.after();
            for (auto& diff : child_diffs) diff.after();

            /* Build the undo/redo collection */
            UndoRedoCollection diff_collection;
            for (auto& [_, diff] : parent_diffs) diff_collection.add_action(std::move(diff));
            for (auto& diff : child_diffs) diff_collection.add_action(std::move(diff));
            diff_collection.commit("Modified Hierarchy");

            ImGui::EndDragDropTarget();
            selected_index_begin = NULL_INDEX;
            selection_parent = entt::null;
            selected_index_end_above = NULL_INDEX;
            selected_index_end_below = NULL_INDEX;
            OnSceneModified::dispatch();
            return true;
        }
        ImGui::EndDragDropTarget();
    }
    return false;
}
void Hierarchy::on_game_end() {
    clear_selection();
}

void Hierarchy::on_pre_unload_scene() {
    clear_selection();
}

}  // namespace tmt