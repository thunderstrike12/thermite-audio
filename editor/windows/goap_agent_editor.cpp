#include "goap_agent_editor.hpp"

#include <imgui.h>

#include "engine/engine.hpp"

#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/world_state.hpp"
#include "engine/systems/ai/goap/components/goap_action_overrides.hpp"
#include "engine/systems/ai/goap/components/goap_goal_registry.hpp"
#include "engine/systems/ai/goap/components/goap_agent_type_ref.hpp"
#include "engine/systems/ai/goap/components/goap_agent_type_registry.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"
#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/core/logger.hpp"

#include <extern/imgui-node-editor/imgui_node_editor.h>
namespace ignode = ax::NodeEditor;
static ignode::EditorContext* g_ContextAgentEditor = nullptr;  // Internal editor state container for imgui-node-editor

namespace tmt {

void GoapAgentEditor::on_editor_start() {
    if (!g_ContextAgentEditor) {
        ignode::Config config;
        config.SettingsFile = "agent_editor.json";
        g_ContextAgentEditor = ignode::CreateEditor(&config);
    }
}

void GoapAgentEditor::on_editor_end() {
    if (g_ContextAgentEditor) {
        ignode::DestroyEditor(g_ContextAgentEditor);
        g_ContextAgentEditor = nullptr;
    }
}

// Editor-local state used for selection and new agent type creation
struct AgentTypeEditorState {
    std::string selected_type;  // Currently selected agent type
    char new_type_name[64] {};  // Buffer for creating a new agent type
};

/**
 * Main display function for the GOAP agent editor window.
 *
 * Displays two panels:
 *   - Left panel: list of existing agent types, add/delete functionality
 *   - Right panel: node editor for editing the selected agent type
 */
void GoapAgentEditor::display() {
    static AgentTypeEditorState state;

    auto* goap = engine.ecs.systems.try_get<Goap>();
    if (!goap) return;

    auto& types = goap->agent_types();

    ImGui::Begin("GOAP Agents");

    // --- Save button ---
    if (ImGui::Button("Save Agent Types")) {
        types.save();
        Log::info("GOAP agent types saved");
    }

    // --- Left panel: Agent type list ---
    ImGui::BeginChild("Types", ImVec2(200, 0), true);

    // Display all registered agent types as selectable items
    for (auto& [id, type] : types.get_all()) {
        if (ImGui::Selectable(id.c_str(), state.selected_type == id)) {
            state.selected_type = id;  // Update selected type
        }
    }

    // --- Delete button ---
    // Only enabled if a type is selected
    if (!state.selected_type.empty()) {
        if (ImGui::Button("Delete Selected Type")) {
            types.remove_type(state.selected_type);  // Remove type from registry
            state.selected_type.clear();             // Clear selection
        }
    }

    ImGui::Separator();

    // --- Add new agent type ---
    ImGui::InputText("New Type", state.new_type_name, 64);  // Input for new type ID
    if (ImGui::Button("Add Agent Type")) {
        if (state.new_type_name[0]) {
            GoapAgentType t;
            t.id = state.new_type_name;     // Assign the new type ID
            types.register_type(t);         // Add to registry
            state.selected_type = t.id;     // Select newly created type
            state.new_type_name[0] = '\0';  // Clear input buffer
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    // --- Right panel: Node editor ---
    ImGui::BeginChild("Editor", ImVec2(0, 0), true);

    if (!state.selected_type.empty()) {
        // Get pointer to selected type
        auto* type = const_cast<GoapAgentType*>(types.get(state.selected_type));
        if (type) {
            draw_agent_type_node(*type);  // Draw the editable node
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

/**
 * Draws a node representing a single agent type.
 *
 * This node displays:
 *   - The agent type ID
 *   - Sections for actions, goals, and default world state
 */
void GoapAgentEditor::draw_agent_type_node(GoapAgentType& type) {
    ignode::SetCurrentEditor(g_ContextAgentEditor);
    ignode::Begin("AgentTypeEditor");

    // Generate a stable node ID from the type's string ID
    int nodeId = (int)(std::hash<std::string>()(type.id) & 0x7fffffff);

    ignode::BeginNode(nodeId);
    ImGui::Text("Agent Type");
    ImGui::Separator();
    ImGui::Text("%s", type.id.c_str());

    // --- Editable sections ---
    draw_actions_section(type);
    draw_goals_section(type);
    draw_world_state_section(type);

    // --- Button to add agents of this type ---
    ImGui::SeparatorText("Runtime Agents");

    if (ImGui::Button("Spawn Agent")) {
        auto& ecs = engine.ecs;

        tmt::Entity e = ecs.create_entity(("Agent_" + type.id).c_str());

        // Add serialized type reference
        auto& type_ref = ecs.add_component<GoapAgentTypeRef>(e);
        type_ref.type_id = type.id;

        Log::info("Placed GOAP agent of type '{}' in scene", type.id);
    }

    ignode::EndNode();

    // Position the node in the editor
    ignode::SetNodePosition(nodeId, ImVec2(100, 100));
    ignode::End();
    ignode::SetCurrentEditor(nullptr);
}

/**
 * Draws the actions section inside an agent type node.
 *
 * Displays all actions from the GOAP system registry as checkboxes.
 * Selected actions are added to the agent type; deselected actions are removed.
 */
void GoapAgentEditor::draw_actions_section(GoapAgentType& type) {
    auto* goap = engine.ecs.systems.try_get<Goap>();
    if (!goap) return;

    ImGui::SeparatorText("Actions");

    // Iterate through all registered actions
    for (auto& [id, action] : goap->actions().get_all()) {
        // Determine if this action is currently part of the type
        bool enabled = std::find(type.action_ids.begin(), type.action_ids.end(), id) != type.action_ids.end();

        // Display checkbox for each action
        if (ImGui::Checkbox(id.c_str(), &enabled)) {
            if (enabled)
                type.action_ids.push_back(id);    // Add to agent type
            else
                std::erase(type.action_ids, id);  // Remove from agent type
        }
    }
}

/**
 * Draws the goals section inside an agent type node.
 *
 * Displays all goals from the GOAP system registry as checkboxes.
 * Selected goals are added to the agent type; deselected goals are removed.
 */
void GoapAgentEditor::draw_goals_section(GoapAgentType& type) {
    auto* goap = engine.ecs.systems.try_get<Goap>();
    if (!goap) return;

    ImGui::SeparatorText("Goals");

    // Iterate through all registered goals
    for (auto& [id, goal] : goap->goals().get_all()) {
        // Determine if this goal is currently part of the type
        bool enabled = std::find(type.goal_ids.begin(), type.goal_ids.end(), id) != type.goal_ids.end();

        // Display checkbox for each goal
        if (ImGui::Checkbox(id.c_str(), &enabled)) {
            if (enabled)
                type.goal_ids.push_back(id);    // Add to agent type
            else
                std::erase(type.goal_ids, id);  // Remove from agent type
        }
    }
}

/**
 * Draws the default world state section inside an agent type node.
 *
 * Allows editing existing facts and adding new facts for the agent type.
 */
void GoapAgentEditor::draw_world_state_section(GoapAgentType& type) {
    ImGui::SeparatorText("Default World State");

    // Iterate through all existing facts in the agent type
    for (auto it = type.default_world_state.begin(); it != type.default_world_state.end();) {
        const std::string& name = it->first;  // The key is now a string directly
        bool v = it->second;

        ImGui::PushID(name.c_str());          // Use string as ImGui ID

        // Display a checkbox for the fact
        if (ImGui::Checkbox(name.c_str(), &v)) it->second = v;

        ImGui::SameLine();

        // Button to remove this fact
        if (ImGui::Button("X")) {
            it = type.default_world_state.erase(it);
        } else {
            ++it;
        }

        ImGui::PopID();
    }

    // --- Add new fact ---
    static char newFact[64] {};
    ImGui::InputText("Add Fact", newFact, 64);
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
        if (newFact[0]) {
            type.default_world_state[newFact] = true;  // key is now string
            newFact[0] = '\0';                         // Clear input buffer
        }
    }
}

}  // namespace tmt
