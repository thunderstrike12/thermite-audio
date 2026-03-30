#include "rig_state_controller.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_node_editor.h>

#include <engine/tools/serializer/containers.hpp>
#include <engine/tools/serializer/rig_controller.hpp>
#include <engine/core/polyline.hpp>

#include "editor.hpp"
#include "editor/windows/hierarchy.hpp"
#include "editor/core/systems/undo_redo/component_diff.hpp"
#include "imgui/extra.hpp"

namespace {

constexpr size_t INVALID_INDEX { std::numeric_limits<size_t>::max() };

ImVec2 get_node_position(const size_t id) {
    return ax::NodeEditor::GetNodePosition(id) + ax::NodeEditor::GetNodeSize(id) / 2.0f;
}

template <typename Type>
[[nodiscard]] std::string make_unique_name(const std::map<std::string, Type>& map, const std::string& string, int index = 1) {
    if (!string.empty() && !map.contains(string)) return string;

    auto format = fmt::format(fmt::runtime(string + " ({})"), index);
    if (!map.contains(format)) return format;

    return make_unique_name(map, string, ++index);
}

bool mouse_clicked_state_node(const size_t node_id, const ImGuiMouseButton button) {
    if (ImGui::IsMouseClicked(button)) {
        // const ImVec2 position = ImNodes::GetNodeScreenSpacePos(node_id);
        // const ImVec2 size = ImNodes::GetNodeDimensions(node_id);
        const ImVec2 position = ax::NodeEditor::GetNodePosition(node_id);
        const ImVec2 size = ax::NodeEditor::GetNodeSize(node_id);
        if (ImGui::IsMouseHoveringRect(position, position + size)) return true;
    }

    return false;
}

const std::map<tmt::Condition::CompareType, std::string> COMPARE_NAMES {
    { tmt::Condition::CompareType::EQUAL, "==" },
    { tmt::Condition::CompareType::NOT_EQUAL, "!=" },
    { tmt::Condition::CompareType::GREATER, ">" },
    { tmt::Condition::CompareType::LESS, "<" },
};

ImRect grid_rect;

bool save_editor(const char* data, size_t size, ax::NodeEditor::SaveReasonFlags, void* user_pointer) {
    const auto entity = static_cast<tmt::Entity>(reinterpret_cast<std::uintptr_t>(user_pointer));
    tmt::RigController& controller = tmt::engine.ecs.get_component<tmt::RigController>(entity);

    controller.editor_data.resize(size);
    std::memcpy(controller.editor_data.data(), data, size);

    return true;
}

size_t load_editor(char* data, void* user_pointer) {
    const auto entity = static_cast<tmt::Entity>(reinterpret_cast<std::uintptr_t>(user_pointer));
    const tmt::RigController& controller = tmt::engine.ecs.get_component<tmt::RigController>(entity);

    if (data != nullptr) std::memcpy(data, controller.editor_data.data(), controller.editor_data.size());

    return controller.editor_data.size();
}

}  // namespace

namespace tmt {

void RigStateController::on_inspect() {
    const Entity selected = editor.systems[Editor::Mode::SCENE].get<Hierarchy>().get_first_selected_entity();
    auto [model, controller] = engine.ecs.get_registry().try_get<RigModel, RigController>(selected);

    const bool valid_controller = (model != nullptr && controller != nullptr && model->data);
    if (!valid_controller) {
        selected_state.clear();
        selected_transition = INVALID_INDEX;
    }

    bool selected_item = false;
    if (valid_controller) selected_item = (controller->states.contains(selected_state) || selected_transition != INVALID_INDEX);

    ComponentDiff<RigController> diff_util { selected };

    const bool has_selection = selected_item && valid_controller;

    ImGui::BeginDisabled(engine.game_controller.is_playing());
    if (ImGui::BeginTable("Rig controller table", 3, ImGuiTableFlags_Resizable, ImVec2 { 0.0f, ImGui::GetContentRegionAvail().y })) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Animations")) {
                ImGui::Separator();

                if (valid_controller) list_animations(*model);

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Parameters")) {
                if (valid_controller) list_parameters(selected, *controller);

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::TableSetColumnIndex(1);

        if (valid_controller) {
            // Start the editor for this controller.
            ax::NodeEditor::SetCurrentEditor(controller->editor_context);

            const ImVec2 start_position = ImGui::GetCursorPos();
            ImGui::Dummy(ImGui::GetContentRegionAvail());
            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Animation name");
                if (payload) {
                    const std::string animation_name {
                        static_cast<const char*>(payload->Data),
                        static_cast<size_t>(payload->DataSize),
                    };

                    diff_util.before();
                    const auto unique_name = make_unique_name(controller->states, animation_name);
                    controller->states[unique_name] = AnimationState { animation_name };

                    const size_t hash = std::hash<std::string> {}(unique_name);
                    ax::NodeEditor::SetNodePosition(hash, ax::NodeEditor::ScreenToCanvas(ImGui::GetMousePos()));
                    diff_util.after();

                    IUndoRedo::send_to_manager(std::move(diff_util), "Create state");
                }

                ImGui::EndDragDropTarget();
            }
            ImGui::SetCursorPos(start_position);

            // Setup grid rect for context menus later
            grid_rect.Min = ImGui::GetCursorPos() + ImGui::GetWindowPos();
            grid_rect.Max = grid_rect.Min + ImGui::GetContentRegionAvail();

            ax::NodeEditor::Begin("NodeEditor");
            draw_transitions(*controller);
            draw_states(selected, *controller);
            ax::NodeEditor::End();

            context_popups(selected, *controller);
        }

        ImGui::TableSetColumnIndex(2);

        if (has_selection) {
            if (!selected_state.empty())
                inspect_state(selected, *controller);
            else if (selected_transition != INVALID_INDEX)
                inspect_transition(selected, *controller);
        }

        ImGui::EndTable();

        ax::NodeEditor::SetCurrentEditor(nullptr);
    }

    ImGui::EndDisabled();
}

void RigStateController::rig_controller_creation(entt::registry& registry, const Entity entity) {
    RigController& controller = registry.get<RigController>(entity);

    ax::NodeEditor::Config config {};
    config.SaveSettings = &save_editor;
    config.LoadSettings = &load_editor;
    config.UserPointer = reinterpret_cast<void*>(static_cast<std::uintptr_t>(entity));
    config.NavigateButtonIndex = 2;  // Set the navigation button to the middle mouse (avoids issues with context menus).

    controller.editor_context = ax::NodeEditor::CreateEditor(&config);
}

void RigStateController::rig_controller_destruction(entt::registry& registry, const Entity entity) {
    const RigController& controller = registry.get<RigController>(entity);

    ax::NodeEditor::DestroyEditor(controller.editor_context);
}

void RigStateController::on_editor_start() {
    engine.ecs.get_registry().on_construct<RigController>().connect<&rig_controller_creation>();
    engine.ecs.get_registry().on_destroy<RigController>().connect<&rig_controller_destruction>();
}

void RigStateController::on_editor_end() {
    engine.ecs.get_registry().on_destroy<RigController>().disconnect<&rig_controller_destruction>();
    engine.ecs.get_registry().on_construct<RigController>().disconnect<&rig_controller_creation>();
}

void RigStateController::list_animations(const RigModel& model) {
    for (auto& [name, animation] : model.data->bones.back().animations) {
        ImGui::Selectable(name.c_str(), false);
        if (ImGui::BeginDragDropSource()) {
            ImGui::TextUnformatted(name.c_str());
            ImGui::SetDragDropPayload("Animation name", name.data(), name.size());
            ImGui::EndDragDropSource();
        }
    }
}

void RigStateController::list_parameters(const Entity entity, RigController& controller) {
    ComponentDiff<RigController> diff_util { entity };

    auto& parameters = controller.parameters;

    static std::string search;
    ImGui::InputText("##search", &search);
    ImGui::SameLine();
    if (ImGui::Button(ICON_MS_ADD)) ImGui::OpenPopup("AddParameter");

    if (ImGui::BeginPopup("AddParameter")) {
        if (ImGui::MenuItem("float")) parameters[make_unique_name(parameters, "New float")] = 0.0f;
        if (ImGui::MenuItem("int")) parameters[make_unique_name(parameters, "New int")] = 0;
        if (ImGui::MenuItem("bool")) parameters[make_unique_name(parameters, "New bool")] = false;
        if (ImGui::MenuItem("trigger")) parameters[make_unique_name(parameters, "New trigger")] = Trigger { false };

        ImGui::EndPopup();
    }

    ImGui::Separator();

    static std::string selected_parameter;
    std::pair<std::string, std::string> edited_map_item;  // Item used to edit the map if the user changed the name of a parameter

    int index = 0;
    for (auto& [name, value] : parameters) {
        if (name.find(search) == std::string::npos) continue;

        ImGui::PushID(index);
        ImGui::BeginGroup();
        std::string editable = name;
        if (ImGui::InputText("##ParameterName", &editable, ImGuiInputTextFlags_EnterReturnsTrue)) edited_map_item = std::make_pair(name, editable);

        ImGui::SameLine();

        const size_t type_index = value.index();
        ImGui::EndDisabled();  // Disabled begin and end are flipped because we want to keep this region enabled within the disabled calls
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        switch (type_index) {
            case 0:            // float
                ImGui::DragFloat("##Parameter", &std::get<float>(value));
                break;
            case 1:            // int
                ImGui::DragInt("##Parameter", &std::get<int>(value));
                break;
            case 2:            // bool
                ImGui::Checkbox("##Parameter", &std::get<bool>(value));
                break;
            case 3: {          // Trigger
                auto& trigger = std::get<Trigger>(value);
                if (ImGui::RadioButton("##Parameter", trigger.value)) trigger.value = !trigger.value;
            } break;

            default:
                break;
        }
        ImGui::BeginDisabled(engine.game_controller.is_playing());
        ImGui::EndGroup();
        ImGui::PopID();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            selected_parameter = name;
            ImGui::OpenPopup("Parameter context");
        }

        index++;
    }

    if (!edited_map_item.first.empty()) {
        diff_util.before();
        controller.rename_parameter(edited_map_item.first, make_unique_name(parameters, edited_map_item.second));
        diff_util.after();
        IUndoRedo::send_to_manager(std::move(diff_util), "Rename parameter");
    } else if (ImGui::BeginPopup("Parameter context")) {
        if (ImGui::MenuItem("Delete parameter")) {
            diff_util.before();
            controller.remove_parameter(selected_parameter);
            diff_util.after();
            IUndoRedo::send_to_manager(diff_util, "Delete parameter");
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RigStateController::draw_transition_select(const ImVec2& middle, const size_t transition_index, const ImVec2& clickable_size = ImVec2 { 25.0f, 25.0f }) {
    const ImVec2 middle_pos = middle - clickable_size / 2.0f;

    ImGui::PushID(static_cast<int>(transition_index & 0xFFFFFFFF));
    ImGui::PushID(static_cast<int>((transition_index >> 32) & 0xFFFFFFFF));

    ImGui::SetCursorPos(middle_pos - ImGui::GetWindowPos());
    if (ImGui::InvisibleButton("##Connection", clickable_size)) {
        selected_state.clear();
        selected_transition = transition_index;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !ImGui::IsPopupOpen("Transition context")) {
        selected_state.clear();
        selected_transition = transition_index;
        open_transition_context = true;
    }

    ImGui::PopID();
    ImGui::PopID();
}

void RigStateController::draw_transitions(const RigController& controller) {
    const auto& transitions = controller.transitions;

    const size_t transition_count = transitions.size();
    for (size_t i = 0; i < transition_count; i++) {
        auto& transition = transitions.at(i);
        if (!positions.contains(transition.from) || !positions.contains(transition.to)) continue;

        ImVec2 start = positions[transition.from];
        ImVec2 middle = (start + positions[transition.to]) / 2.0f;

        const auto find_opposite_transition = std::ranges::find_if(transitions, [transition](const Transition& other) { return transition.from == other.to && transition.to == other.from; });
        if (find_opposite_transition != transitions.end()) {
            const glm::vec2 swizzle = glm::normalize(glm::vec2 { -(middle.y - start.y), middle.x - start.x });
            start += ImVec2 { swizzle.x, swizzle.y } * 10.0f;
            middle += ImVec2 { swizzle.x, swizzle.y } * 10.0f;
        }

        draw_transition(start, middle, i);
        draw_transition_select(middle, i);
    }

    if (!connecting_state.empty()) {
        const ImVec2 start = positions[connecting_state];
        draw_transition(start, (start + ImGui::GetMousePos()) / 2.0f, INVALID_INDEX);
    }
}

void RigStateController::draw_transition(const ImVec2& start, const ImVec2& middle, const size_t transition_index) const {
    constexpr float triangle_size = 12.0f;
    ImU32 color = 0xFFFFFFFF;
    if (transition_index == selected_transition) color = 0xFFEE9A39;  // Selection blue

    // Fancy math is to calculate a regular triangle that is actually in the middle of the line and looks good
    const ImVec2 vector = middle - start;
    ImGui::GetWindowDrawList()->AddLine(start, start + (vector * 2.0f), color, 5.0f);

    const glm::vec2 direction = normalize(glm::vec2 { vector.x, vector.y });

    const float angle = glm::atan(direction.x, direction.y);
    constexpr float offset_angle = glm::two_pi<float>() / 3.0f;

    const ImVec2 forward_point = middle + ImVec2 { glm::sin(angle), glm::cos(angle) } * triangle_size;
    const ImVec2 left_point = middle + ImVec2 { glm::sin(angle - offset_angle), glm::cos(angle - offset_angle) } * triangle_size;
    const ImVec2 right_point = middle + ImVec2 { glm::sin(angle + offset_angle), glm::cos(angle + offset_angle) } * triangle_size;

    ImGui::GetWindowDrawList()->AddTriangleFilled(forward_point, left_point, right_point, color);
}

void RigStateController::draw_states(const Entity entity, RigController& controller) {
    positions.clear();
    for (auto& [name, state] : controller.states) {
        const size_t hash = std::hash<std::string> {}(name);

        if (name == controller.current_state) {
            ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg, ImGui::ColorConvertU32ToFloat4(0xFF177032));
            ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeSelRect, ImGui::ColorConvertU32ToFloat4(0xFF26BC54));
        }

        ax::NodeEditor::BeginNode(hash);
        ImGui::TextUnformatted(name.c_str());

        if (mouse_clicked_state_node(hash, ImGuiMouseButton_Left)) {
            if (!connecting_state.empty()) {
                const auto exists_iterator =
                    std::ranges::find_if(controller.transitions, [this, name](const Transition& transition) { return transition.from == connecting_state && transition.to == name; });
                if (exists_iterator == controller.transitions.end() && connecting_state != name) {
                    ComponentDiff<RigController> diff_util { entity };
                    diff_util.before();
                    controller.transitions.push_back(Transition { connecting_state, name });
                    diff_util.after();
                    IUndoRedo::send_to_manager(diff_util, "Create transition");
                }

                connecting_state.clear();
            } else {
                selected_transition = INVALID_INDEX;
                selected_state = name;
            }
        }

        if (mouse_clicked_state_node(hash, ImGuiMouseButton_Right) && !ImGui::IsPopupOpen("State context")) {
            selected_transition = INVALID_INDEX;
            selected_state = name;
            open_state_context = true;
        }

        positions[name] = get_node_position(hash);
        ax::NodeEditor::EndNode();

        if (name == controller.current_state) ax::NodeEditor::PopStyleColor(2);
    }

    // Clicked while connecting, but no node is being clicked
    if (!connecting_state.empty() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) connecting_state.clear();
}

void RigStateController::context_popups(const Entity entity, RigController& controller) {
    ComponentDiff<RigController> diff_util { entity };

    if (open_state_context) {
        open_state_context = false;
        ImGui::OpenPopup("State context");
    } else if (open_transition_context) {
        open_transition_context = false;
        ImGui::OpenPopup("Transition context");
    } else if (connecting_state.empty() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && grid_rect.Contains(ImGui::GetMousePos())) {
        ImGui::OpenPopup("General context");
    }

    if (ImGui::BeginPopup("State context")) {
        if (!selected_state.empty()) {
            if (ImGui::MenuItem("Connect state")) {
                connecting_state = selected_state;
                ImGui::CloseCurrentPopup();
            }

            if (selected_state != "Start" && ImGui::MenuItem("Delete state")) {
                diff_util.before();

                ax::NodeEditor::ClearSelection();

                controller.remove_state(selected_state);
                if (controller.current_state == selected_state) controller.current_state.clear();

                diff_util.after();
                IUndoRedo::send_to_manager(diff_util, "Delete state");

                selected_state.clear();
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Transition context")) {
        if (selected_transition != INVALID_INDEX) {
            if (ImGui::MenuItem("Delete transition")) {
                diff_util.before();
                controller.transitions.erase(controller.transitions.begin() + static_cast<int64_t>(selected_transition));
                selected_transition = INVALID_INDEX;
                diff_util.after();
                IUndoRedo::send_to_manager(diff_util, "Delete transition");
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("General context")) {
        if (ImGui::MenuItem("Create state")) {
            diff_util.before();
            const std::string state_name = make_unique_name(controller.states, "New state");
            const size_t hash = std::hash<std::string> {}(state_name);
            controller.states[state_name];

            ax::NodeEditor::SetNodePosition(hash, ax::NodeEditor::ScreenToCanvas(ImGui::GetMousePos()));

            diff_util.after();
            IUndoRedo::send_to_manager(std::move(diff_util), "Create state");
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RigStateController::inspect_state(const Entity entity, RigController& controller) {
    if (selected_state == "Start") return;
    static std::shared_ptr<ComponentDiff<RigController>> state_diff {};
    auto& state = controller.states[selected_state];

    ImGui::SeparatorText("State settings: ");

    ImGui::SameLine();

    std::string editable = selected_state;
    if (ImGui::InputText("##ParameterName", &editable, ImGuiInputTextFlags_EnterReturnsTrue)) {
        const std::string& new_name = make_unique_name(controller.states, editable);

        ComponentDiff<RigController> diff_util { entity };
        diff_util.before();
        controller.rename_state(selected_state, new_name);

        // Sets the node position properly to not launch the node to (0, 0) when the state is renamed;
        constexpr std::hash<std::string> hasher;
        // const ImVec2 node_position = ImNodes::GetNodeScreenSpacePos(static_cast<int>(hasher(selected_state)));
        // ImNodes::SetNodeScreenSpacePos(static_cast<int>(hasher(new_name)), node_position);
        const ImVec2 node_position = ax::NodeEditor::GetNodePosition(hasher(selected_state));
        ax::NodeEditor::SetNodePosition(hasher(new_name), node_position);

        IUndoRedo::send_to_manager(diff_util, "Rename state");
        selected_state = new_name;
    }

    ImGui::BeginGroup();

    ImGui::InputText("Animation", &state.animation, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Animation name");
        if (payload) {
            state_diff->before();
            const std::string_view animation_name {
                static_cast<const char*>(payload->Data),
                static_cast<const char*>(payload->Data) + payload->DataSize,
            };
            state.animation = animation_name;
            state_diff->after();

            IUndoRedo::send_to_manager(std::move(*state_diff), "Edit state animation");
            state_diff.reset();
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::DragFloat("Animation speed", &state.animation_speed);
    ImGui::Checkbox("Repeat", &state.repeat);
    ImGui::Checkbox("Only on finish", &state.transition_on_finish);
    tooltip("Only transition to the next state when the animation has fully played.");

    ImGui::EndGroup();

    if (ImGui::IsItemActivated()) {
        state_diff = std::make_shared<ComponentDiff<RigController>>(entity);
        state_diff->before();
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        state_diff->after();

        IUndoRedo::send_to_manager(std::move(*state_diff), "Edit state");
        state_diff.reset();
    }
}

void RigStateController::inspect_transition(const Entity entity, RigController& controller) const {
    static std::shared_ptr<ComponentDiff<RigController>> diff_util {};
    auto& transition = controller.transitions[selected_transition];

    ImGui::SeparatorText("Transition settings");

    const float text_input_width = (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(ICON_MS_ARROW_RIGHT).x) / 2.0f;
    ImGui::PushItemWidth(text_input_width);
    ImGui::InputText("##From", &transition.from, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();

    ImGui::Text(ICON_MS_ARROW_RIGHT);
    ImGui::SameLine();

    ImGui::PushItemWidth(text_input_width);
    ImGui::InputText("##To", &transition.to, ImGuiInputTextFlags_ReadOnly);
    ImGui::Spacing();

    ImGui::DragFloat("Transition time", &transition.transition_time);
    if (ImGui::IsItemActivated()) {
        diff_util = std::make_shared<ComponentDiff<RigController>>(entity);
        diff_util->before();
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        diff_util->after();

        IUndoRedo::send_to_manager(std::move(*diff_util), "Transition edited");
        diff_util.reset();
    }

    ImGui::NewLine();

    list_conditions(entity, controller, transition);
}

void RigStateController::list_conditions(const Entity entity, RigController& controller, Transition& transition) {
    static std::shared_ptr<ComponentDiff<RigController>> diff_util {};
    static size_t condition_index = INVALID_INDEX;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);
    ImGui::BeginChild("Conditions", ImVec2 { 0.0f, 0.0f }, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar()) {
        ImGui::Text("Conditions");
        ImGui::EndMenuBar();
    }

    const size_t condition_count = transition.conditions.size();
    for (size_t i = 0; i < condition_count; i++) {
        auto& condition = transition.conditions[i];

        ImGui::PushID(static_cast<int>(i & 0xFFFFFFFF));
        ImGui::PushID(static_cast<int>((i >> 32) & 0xFFFFFFFF));

        ImGui::BeginGroup();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::BeginCombo("##Parameter name", condition.parameter_name.c_str())) {
            for (auto& [name, value] : controller.parameters) {
                if (ImGui::Selectable(name.c_str(), false)) {
                    condition.parameter_name = name;
                    condition.compare_value = value;
                    if (value.index() == 0)  // Float type
                        condition.compare_type = Condition::CompareType::GREATER;
                    else
                        condition.compare_type = Condition::CompareType::EQUAL;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();

        const size_t type_index = condition.compare_value.index();
        switch (type_index) {
            case 0:  // float
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                if (ImGui::BeginCombo("##Comparison type", COMPARE_NAMES.at(condition.compare_type).c_str())) {
                    if (ImGui::Selectable(">", false)) condition.compare_type = Condition::CompareType::GREATER;
                    if (ImGui::Selectable("<", false)) condition.compare_type = Condition::CompareType::LESS;
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat("##Parameter", &std::get<float>(condition.compare_value));
                break;
            case 1:  // int
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                if (ImGui::BeginCombo("##Comparison type", COMPARE_NAMES.at(condition.compare_type).c_str())) {
                    if (ImGui::Selectable("==", false)) condition.compare_type = Condition::CompareType::EQUAL;
                    if (ImGui::Selectable("!=", false)) condition.compare_type = Condition::CompareType::NOT_EQUAL;
                    if (ImGui::Selectable(">", false)) condition.compare_type = Condition::CompareType::GREATER;
                    if (ImGui::Selectable("<", false)) condition.compare_type = Condition::CompareType::LESS;
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragInt("##Parameter", &std::get<int>(condition.compare_value));
                if (ImGui::IsItemActivated()) {
                    diff_util = std::make_shared<ComponentDiff<RigController>>(entity);
                    diff_util->before();
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    diff_util->after();

                    IUndoRedo::send_to_manager(std::move(*diff_util), "Condition value edited");
                    diff_util.reset();
                }
                break;
            case 2: {  // bool
                auto& value = std::get<bool>(condition.compare_value);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo("##Parameter", (value ? "true" : "false"))) {
                    if (ImGui::Selectable("true", false)) value = true;
                    if (ImGui::Selectable("false", false)) value = false;
                    ImGui::EndCombo();
                }
            } break;
            case 3:  // trigger
                ImGui::NewLine();
                break;
            default:
                break;
        }
        ImGui::EndGroup();

        ImGui::PopID();
        ImGui::PopID();

        if (ImGui::IsItemActivated()) {
            diff_util = std::make_shared<ComponentDiff<RigController>>(entity);
            diff_util->before();
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            diff_util->after();

            IUndoRedo::send_to_manager(std::move(*diff_util), "Condition edited");
            diff_util.reset();

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                condition_index = i;
                ImGui::OpenPopup("Condition context");
            }
        }

        if (ImGui::BeginPopup("Condition context")) {
            if (ImGui::MenuItem("Delete condition")) {
                ComponentDiff<RigController> diff { entity };

                diff.before();
                transition.conditions.erase(transition.conditions.begin() + static_cast<int64_t>(condition_index));
                diff.after();

                IUndoRedo::send_to_manager(diff, "Delete condition");
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::BeginDisabled(controller.parameters.empty());
    if (ImGui::Button(ICON_MS_ADD)) {
        auto& [name, value_variant] = *controller.parameters.begin();
        const size_t type_index = value_variant.index();

        Condition condition;
        switch (type_index) {
            case 0:  // Float
                condition = Condition { name, 0.0f, Condition::CompareType::GREATER };
                break;
            case 1:  // Int
                condition = Condition { name, 0, Condition::CompareType::EQUAL };
                break;
            case 2:  // Bool
                condition = Condition { name, true, Condition::CompareType::EQUAL };
                break;
            case 3:  // Trigger
                condition = Condition { name, Trigger { false }, Condition::CompareType::EQUAL };
            default:
                break;
        }

        ComponentDiff<RigController> diff { entity };

        diff.before();
        transition.conditions.push_back(condition);
        diff.after();

        IUndoRedo::send_to_manager(std::move(diff), "Condition added");
    }
    ImGui::EndDisabled();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

}  // namespace tmt