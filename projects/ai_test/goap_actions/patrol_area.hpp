#pragma once
#include "engine/systems/ai/components/goap_action.hpp"

namespace tmt {

class PatrolArea : public GoapAction {
   public:
    PatrolArea() {
        // Preconditions: player not visible
        preconditions["player_visible"] = false;

        // Effects: area secure
        effects["area_secure"] = true;

        cost = 4.f;
    }

    const char* get_name() const override { return "PatrolArea"; }
};

}  // namespace tmt
