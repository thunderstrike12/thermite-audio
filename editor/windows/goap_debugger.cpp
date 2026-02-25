#include "goap_debugger.hpp"

#include <imgui.h>

#include "engine/engine.hpp"

#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/world_state.hpp"
#include "engine/systems/ai/goap/components/goap_action_overrides.hpp"
#include "engine/systems/ai/goap/components/goap_goal_registry.hpp"
#include "engine/systems/ai/goap/components/goap_agent_type_registry.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"
#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/core/logger.hpp"

#include <extern/imgui-node-editor/imgui_node_editor.h>
namespace ignode = ax::NodeEditor;
static ignode::EditorContext* g_ContextDebugger = nullptr;  // internal state container for imgui-node-editor.

namespace tmt {

void GoapDebugger::on_editor_start() {
    if (!g_ContextDebugger) {
        ignode::Config config;
        config.SettingsFile = "goap_graph.json";
        g_ContextDebugger = ignode::CreateEditor(&config);
    }
}

void GoapDebugger::on_editor_end() {
    if (g_ContextDebugger) {
        ignode::DestroyEditor(g_ContextDebugger);
        g_ContextDebugger = nullptr;
    }
}

/**
 * Main display function for the GOAP Debugger window.
 *
 *   - Allow selecting a GOAP agent from all entities in the ECS.
 *   - Display detailed information about the selected agent:
 *       - Active goal
 *       - Current plan with steps, preconditions, and effects
 *       - World state facts
 *       - Available goals and actions
 *   - Render a visual graph of the agent's plan and available actions/goals.
 */
void GoapDebugger::display() {
    auto& ecs = engine.ecs.get_registry();
    static Entity selected_agent = entt::null;

    // Gather all agents
    std::vector<Entity> agents;
    for (auto [e, agent] : engine.ecs.view<GoapAgent>().each()) {
        agents.push_back(e);
    }

    ImGui::Begin("GOAP Debugger");

    // --- Agent Selector ---
    if (!agents.empty()) {
        static std::vector<std::string> agent_name_storage;
        agent_name_storage.clear();

        std::vector<const char*> agent_names;
        for (auto a : agents) {
            agent_name_storage.emplace_back("Agent " + std::to_string((uint32_t)a));
            agent_names.push_back(agent_name_storage.back().c_str());
        }

        static int current_index = 0;
        if (selected_agent != entt::null) {
            for (size_t i = 0; i < agents.size(); ++i) {
                if (agents[i] == selected_agent) current_index = (int)i;
            }
        }

        if (agents.size() == 1) {
            // Only one agent, just display its name
            selected_agent = agents[0];
            ImGui::Text("Agent: %s", agent_names[0]);
        } else {
            // Multiple agents - show dropdown
            static const char* combo_preview_val = "Select Agent..";
            if (ImGui::BeginCombo("Select Agent", combo_preview_val)) {
                for (size_t i = 0; i < agents.size(); ++i) {
                    const char* name = agent_names[i];
                    if (ImGui::Selectable(name, i == current_index)) {
                        selected_agent = agents[i];
                        current_index = (int)i;
                        combo_preview_val = name;
                    }
                }
                ImGui::EndCombo();
            }
        }
    }

    // --- Draw only selected agent ---
    if (selected_agent != entt::null && ecs.all_of<GoapAgent, WorldState>(selected_agent)) {
        auto& agent = ecs.get<GoapAgent>(selected_agent);
        auto& ws = ecs.get<WorldState>(selected_agent);

        ImGui::BeginChild("Details", ImVec2(400, 0), true);
        draw_details_view(agent, ws);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Graph", ImVec2(0, 0), true);
        draw_goap_graph(agent, ws);
        ImGui::EndChild();
    }

    ImGui::End();
}

/**
 * Draws the details view of a GOAP agent in the editor.
 *
 * Sections displayed:
 *   - Active goal
 *   - Plan steps with cost, preconditions, effects, and current action highlight
 *   - World state facts with editable checkboxes for boolean values (for testing purposes)
 *   - Available goals with priority and relevance
 *   - Available actions with cost and precondition checks
 *
 * Allows forcing a manual replan if needed.
 */
void GoapDebugger::draw_details_view(GoapAgent& agent, WorldState& ws) {
    // auto& overrides = GoapActionOverrides::instance();
    auto& ecs = engine.ecs;

    // --- Get GOAP system ---
    Goap* goap = ecs.systems.try_get<Goap>();
    if (!goap) {
        Log::warn("GOAP system not active.");
        return;
    }

    auto& overrides = goap->overrides();

    // --- Active goal ---
    if (agent.has_goal()) {
        ImGui::Text("Active Goal: %s", agent.active_goal.name.c_str());
    } else {
        ImGui::Text("Active Goal: <none>");
    }

    // --- Plan ---
    if (ImGui::TreeNode("Plan")) {
        for (int i = 0; i < (int)agent.plan.size(); ++i) {
            bool current = (i == agent.current_index);

            // --- Merge overrides ---
            auto* override = overrides.find(agent.plan[i]->get_id());
            EffectiveGoapAction effective = build_effective_action(*agent.plan[i], override);

            ImGui::Text("%s [cost: %.1f] %s", agent.plan[i]->get_id().c_str(), effective.cost, current ? "-> CURRENT" : "");

            // --- Preconditions per plan node ---
            if (ImGui::TreeNode((std::string("Preconditions##") + std::to_string((uintptr_t)agent.plan[i])).c_str())) {
                for (auto& [key, val] : effective.preconditions) {
                    ImGui::Text("%s = %s", key.c_str(), val ? "true" : "false");
                }
                ImGui::TreePop();
            }

            // --- Effects per plan node ---
            if (ImGui::TreeNode((std::string("Effects##") + std::to_string((uintptr_t)agent.plan[i])).c_str())) {
                for (auto& [key, val] : effective.effects) {
                    ImGui::Text("%s = %s", key.c_str(), val ? "true" : "false");
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    // --- World state ---
    if (ImGui::TreeNode("World State")) {
        for (auto& [id, val] : ws.facts) {
            const std::string& fact_name = FactRegistry::instance().get_name(id);

            ImGui::PushID(id);

            if (ImGui::Checkbox(fact_name.c_str(), &val)) {
                agent.needs_replan = true;
            }

            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    // --- Available goals ---
    if (ImGui::TreeNode("Available Goals")) {
        for (auto& goal : agent.available_goals) {
            ImGui::Text("%s [priority: %d] %s", goal.name.c_str(), goal.priority, goal.is_relevant(ws) ? "(relevant)" : "(satisfied)");
        }
        ImGui::TreePop();
    }

    // --- Available actions ---
    if (ImGui::TreeNode("Available Actions")) {
        for (const GoapAction* action : agent.available_actions) {
            if (!action) continue;

            auto* override = overrides.find(action->get_id());
            EffectiveGoapAction effective = build_effective_action(*action, override);

            bool can_run = true;
            for (auto& [fact, val] : effective.preconditions) {
                auto it = ws.facts.find((uint32_t)std::hash<std::string>()(fact));
                if (it == ws.facts.end() || it->second != val) {
                    can_run = false;
                    break;
                }
            }

            ImGui::Text("%s [cost: %.1f] %s", action->get_id().c_str(), effective.cost, can_run ? "(can run)" : "(cannot run)");

            // --- Preconditions ---
            if (ImGui::TreeNode((std::string("Preconditions##") + std::to_string((uintptr_t)action)).c_str())) {
                for (auto& [key, val] : effective.preconditions) {
                    ImGui::Text("%s = %s", key.c_str(), val ? "true" : "false");
                }
                ImGui::TreePop();
            }

            // --- Effects ---
            if (ImGui::TreeNode((std::string("Effects##") + std::to_string((uintptr_t)action)).c_str())) {
                for (auto& [key, val] : effective.effects) {
                    ImGui::Text("%s = %s", key.c_str(), val ? "true" : "false");
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    // --- Manual replan ---
    if (ImGui::Button("Force Replan")) {
        agent.needs_replan = true;
    }
}

struct PlanNode {
    int node;
    int inPin;
    int outPin;
};

/**
 * Draws a visual graph of the agent's current plan, available actions, and goals.
 *
 * Display:
 *   - Plan sequence as nodes connected from start to active goal
 *   - Available actions with preconditions and effects
 *   - Available goals with priority and relevance
 */
void GoapDebugger::draw_goap_graph(GoapAgent& agent, WorldState& ws) {
    ignode::SetCurrentEditor(g_ContextDebugger);
    ignode::Begin("GOAP Graph");

    int nodeId = 1000;
    int pinId = 2000;
    int linkId = 3000;

    const float planY = 0.0f;
    const float columnsY = 260.0f;

    const float colSpacing = 260.0f;
    const float rowSpacing = 260.0f;

    float planX = 0.0f;

    std::vector<PlanNode> planNodes;

    // --- Plan ---
    for (int i = 0; i < (int)agent.plan.size(); ++i) {
        PlanNode n { nodeId++, pinId++, pinId++ };
        planNodes.push_back(n);

        ignode::BeginNode(n.node);

        ImGui::Text("%s", agent.plan[i]->get_id().c_str());

        if (i == agent.current_index) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 50, 255));
            ImGui::Text("-> CURRENT");
            ImGui::PopStyleColor();
        }

        ignode::BeginPin(n.inPin, ignode::PinKind::Input);
        ignode::EndPin();
        ignode::BeginPin(n.outPin, ignode::PinKind::Output);
        ignode::EndPin();

        ignode::EndNode();
        ignode::SetNodePosition(n.node, ImVec2(planX + i * colSpacing, planY));
    }

    // --- Active goal ---
    int goalNode = nodeId++;
    int goalPin = pinId++;

    ignode::BeginNode(goalNode);
    ImGui::Text("%s", agent.active_goal.name.c_str());
    ignode::BeginPin(goalPin, ignode::PinKind::Input);
    ignode::EndPin();
    ignode::EndNode();

    ignode::SetNodePosition(goalNode, ImVec2(planX + agent.plan.size() * colSpacing, planY));

    // --- Plan links ---
    for (size_t i = 0; i + 1 < planNodes.size(); ++i) ignode::Link(linkId++, planNodes[i].outPin, planNodes[i + 1].inPin);

    if (!planNodes.empty()) ignode::Link(linkId++, planNodes.back().outPin, goalPin);

    // --- Actions + goals nodes ---
    float actionsX = 0.0f;
    float goalsX = actionsX + colSpacing;

    size_t rows = std::max(agent.available_actions.size(), agent.available_goals.size());

    for (size_t i = 0; i < rows; ++i) {
        float y = columnsY + i * rowSpacing;

        // ----- actions -----
        if (i < agent.available_actions.size()) {
            GoapAction* a = agent.available_actions[i];
            if (a) {
                int n = nodeId++;
                ignode::BeginNode(n);

                ImGui::Text("Action");
                ImGui::Separator();
                ImGui::Text("%s", a->get_id().c_str());
                ImGui::Text("Cost: %.1f", a->cost);

                ImGui::Text("Preconditions:");
                for (auto& [k, v] : a->preconditions) ImGui::BulletText("%s", k.c_str());

                ImGui::Text("Effects:");
                for (auto& [k, v] : a->effects) ImGui::BulletText("%s", k.c_str());
                ignode::EndNode();
                ignode::SetNodePosition(n, ImVec2(actionsX, y));
            }
        }

        // ----- Goals -----
        if (i < agent.available_goals.size()) {
            auto& g = agent.available_goals[i];
            int n = nodeId++;

            ignode::BeginNode(n);
            ImGui::Text("Goal");
            ImGui::Separator();
            ImGui::Text("%s", g.name.c_str());
            ImGui::Text("Priority: %d", g.priority);
            ImGui::Text("Relevance: %s", g.is_relevant(ws) ? "(relevant)" : "(satisfied)");
            ignode::EndNode();

            ignode::SetNodePosition(n, ImVec2(goalsX, y));
        }
    }

    // --- Camera ---
    static bool firstFrame = true;
    if (firstFrame) {
        ignode::NavigateToContent();
        firstFrame = false;
    }

    ignode::End();
    ignode::SetCurrentEditor(nullptr);
}

}  // namespace tmt
