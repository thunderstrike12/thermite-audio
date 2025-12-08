#pragma once
#include "engine/core/ecs.hpp"
#include <vector>
#include "goap_action.hpp"
#include "goap_goal.hpp"

namespace tmt {

struct GoapAgent {
    std::vector<std::unique_ptr<GoapAction>> actions;

    std::vector<GoapGoal> available_goals;
    GoapGoal active_goal;

    std::vector<GoapAction*> plan;
    int current_index = -1;
    bool needs_replan = true;

    GoapAgent() = default;

    // Prevent copying
    GoapAgent(const GoapAgent&) = delete;
    GoapAgent& operator=(const GoapAgent&) = delete;

    // Reset the current plan
    void clear_plan() {
        plan.clear();
        current_index = -1;
    }

    bool has_goal() const { return active_goal.valid; }

    // Get pointer to current action, or nullptr
    GoapAction* get_current_action() {
        if (current_index < 0 || current_index >= (int)plan.size()) return nullptr;
        return plan[current_index];
    }
};

}  // namespace tmt
