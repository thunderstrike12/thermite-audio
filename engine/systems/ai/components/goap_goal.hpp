#pragma once
#include "world_state.hpp"
#include <vector>

namespace tmt {

struct GoapGoal {
    std::vector<FactPair> desired_state;
    int priority = 0;
};

}  // namespace tmt
