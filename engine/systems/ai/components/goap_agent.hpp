#pragma once
#include "core/ecs.hpp"
#include <vector>
#include "goap_action.hpp"

namespace tmt {

struct GoapAgent {
    std::vector<std::unique_ptr<GoapAction>> actions;
    std::vector<GoapAction*> plan;
    int current_index = -1;

    bool needs_replan = true;
    bool has_goal = false;

    GoapAgent() = default;

    // Prevent copying
    GoapAgent(const GoapAgent&) = delete;
    GoapAgent& operator=(const GoapAgent&) = delete;

    // Reset the current plan
    void clear_plan() {
        plan.clear();
        current_index = -1;
    }

    // Get pointer to current action, or nullptr
    GoapAction* get_current_action() {
        if (current_index < 0 || current_index >= (int)plan.size()) return nullptr;
        return plan[current_index];
    }
};

}  // namespace tmt
