#pragma once
#include "world_state.hpp"
#include <vector>

/**
 * Struct GoapGoal
 * Defines a desired end state the agent attempts to reach.
 *
 * Each goal has:
 *   - a name
 *   - desired world-state facts
 *   - priority (used to choose goals, higher values = more important, more likely to be selected)
 *
 * Goals are relevant if their desired state is not already satisfied.
 */

namespace tmt {

struct GoapGoal {
    std::string name;
    std::vector<FactPair> desired_state;
    int priority = 0;
    bool valid = false;

    /**
     * Returns true if the goal should be activated.
     *
     * A goal is relevant when:
     *    - It is marked valid
     *    - Its desired state is not satisfied
     */
    bool is_relevant(const WorldState& ws) const {
        if (!valid) return false;
        return !ws.satisfies(desired_state);
    }
};

}  // namespace tmt
