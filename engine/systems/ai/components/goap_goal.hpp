#pragma once
#include "world_state.hpp"
#include <vector>

namespace tmt {

struct GoapGoal {
    std::string name;
    std::vector<FactPair> desired_state;
    int priority = 0;
    bool valid = false;

    // A goal is relevant if it is not already satisfied by the world state
    bool is_relevant(const WorldState& ws) const {
        if (!valid) return false;
        return !ws.satisfies(desired_state);
    }
};

}  // namespace tmt
