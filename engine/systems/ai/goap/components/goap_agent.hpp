#pragma once
#include "engine/core/ecs.hpp"
#include <vector>
#include "goap_action.hpp"
#include "goap_goal.hpp"

/**
 * Struct GoapAgent
 * Stores all data required for GOAP planning and execution.
 *
 * Contains:
 *   - Available actions
 *   - Current running action
 *   - Available goals
 *   - Active goal
 *   - Current plan (sequence of actions)
 *   - Execution cursor
 *   - Flags for replanning
 */

namespace tmt {

struct GoapAgent {
    // Owned actions available to this agent.
    std::vector<GoapAction*> available_actions;

    // Decleration of what actions this agent supports.
    std::vector<std::string> action_set;

    // Currently executing action (nullptr if idle).
    GoapAction* current_action = nullptr;

    // Goals the agent may select from.
    std::vector<GoapGoal> available_goals;

    // Selected active goal.
    GoapGoal active_goal;

    // Ordered list of actions built by the planner.
    std::vector<GoapAction*> plan;

    // Index of current action in plan.
    int current_index = -1;

    // Whether the agent should rebuild its plan.
    bool needs_replan = true;

    GoapAgent() = default;
    GoapAgent(const GoapAgent&) = delete;  // Prevent copying.
    GoapAgent& operator=(const GoapAgent&) = delete;

    // Clears the current action plan.
    void clear_plan() {
        plan.clear();
        current_index = -1;
    }

    // Whether a valid goal is currently selected.
    bool has_goal() const { return active_goal.valid; }

    // Gets the current action or nullptr.
    GoapAction* get_current_action() {
        if (current_index < 0 || current_index >= (int)plan.size()) return nullptr;
        return plan[current_index];
    }
};

}  // namespace tmt
