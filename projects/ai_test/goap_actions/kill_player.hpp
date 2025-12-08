#pragma once
#include "engine/systems/ai/components/goap_action.hpp"

namespace tmt {

class KillPlayer : public GoapAction {
   public:
    KillPlayer() {
        // Preconditions: player must be in range and alive
        preconditions["player_in_range"] = true;
        preconditions["player_alive"] = true;

        // Effects: player dead, area secure
        effects["player_alive"] = false;
        effects["area_secure"] = true;

        cost = 2.f;
    }

    const char* get_name() const override { return "KillPlayer"; }
};

}  // namespace tmt
