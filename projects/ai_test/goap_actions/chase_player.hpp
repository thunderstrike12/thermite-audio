#pragma once
#pragma once
// #include "engine/core/ecs.hpp"
#include "engine/systems/ai/components/goap_action.hpp"

namespace tmt {

class ChasePlayer : public GoapAction {
   public:
    ChasePlayer() {
        // Preconditions: player must be visible
        preconditions["player_visible"] = true;

        // Effects: player in range
        effects["player_in_range"] = true;

        cost = 1.f;
    }

    const char* get_name() const override { return "ChasePlayer"; }
};

}  // namespace tmt
