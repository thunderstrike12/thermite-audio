#include "goap_debugger.hpp"

#include <imgui.h>

#include "engine/engine.hpp"

#include "engine/systems/ai/components/goap_agent.hpp"
#include "engine/systems/ai/components/world_state.hpp"

namespace tmt {

void GoapDebugger::display() {
    auto& ecs = engine.ecs.get_registry();

    if (ImGui::Begin("GOAP Debugger")) {
        ecs.view<GoapAgent, WorldState>().each([&](Entity e, GoapAgent& agent, WorldState& ws) {
            ImGui::SeparatorText(("Agent " + std::to_string((uint32_t)e)).c_str());

            // Active goal
            if (agent.has_goal()) {
                ImGui::Text("Active Goal: %s", agent.active_goal.name.c_str());
            } else {
                ImGui::Text("Active Goal: <none>");
            }

            // Plan
            if (ImGui::TreeNode("Plan")) {
                for (int i = 0; i < (int)agent.plan.size(); ++i) {
                    bool current = (i == agent.current_index);
                    ImGui::Text("%s %s", current ? "->" : " ", agent.plan[i]->get_name());
                }
                ImGui::TreePop();
            }

            // World state
            if (ImGui::TreeNode("World State")) {
                for (auto& [id, val] : ws.facts) {
                    const std::string& fact_name = FactRegistry::instance().get_name(id);

                    ImGui::PushID(id);  // ensure unique ImGui ID per fact

                    switch (val.value_type) {
                        case FactValue::Type::BOOL_TYPE: {
                            bool old = val.bool_val;
                            if (ImGui::Checkbox(fact_name.c_str(), &val.bool_val)) {
                                agent.needs_replan = true;
                            }
                            break;
                        }

                        case FactValue::Type::INT_TYPE:
                            ImGui::Text("%s = %d", fact_name.c_str(), val.int_val);
                            break;

                        case FactValue::Type::FLOAT_TYPE:
                            ImGui::Text("%s = %.2f", fact_name.c_str(), val.float_val);
                            break;
                    }

                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            // Available goals
            if (ImGui::TreeNode("Available Goals")) {
                for (auto& goal : agent.available_goals) {
                    bool relevant = goal.is_relevant(ws);
                    ImGui::Text("%s [priority: %d] %s", goal.name.c_str(), goal.priority, relevant ? "(relevant)" : "(satisfied)");
                }
                ImGui::TreePop();
            }

            // Available actions
            if (ImGui::TreeNode("Available Actions")) {
                for (auto& action_ptr : agent.actions) {
                    GoapAction* action = action_ptr.get();  // get the raw pointer
                    if (!action) continue;

                    bool can_run = action->check_preconditions(ws);
                    ImGui::Text("%s [cost: %.1f] %s", action->get_name(), action->cost, can_run ? "(can run)" : "(cannot run)");

                    // Preconditions
                    if (ImGui::TreeNode((std::string("Preconditions##") + std::to_string((uintptr_t)action)).c_str())) {
                        for (auto& [key, val] : action->preconditions) {
                            ImGui::Text("%s = %s", key.c_str(), val ? "true" : "false");
                        }
                        ImGui::TreePop();
                    }

                    // Effects
                    if (ImGui::TreeNode((std::string("Effects##") + std::to_string((uintptr_t)action)).c_str())) {
                        for (auto& [key, val] : action->effects) {
                            ImGui::Text("%s = %s", key.c_str(), val ? "true" : "false");
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            // Manual replan
            if (ImGui::Button("Force Replan")) {
                agent.needs_replan = true;
            }
        });
    }
    ImGui::End();
}

}  // namespace tmt
