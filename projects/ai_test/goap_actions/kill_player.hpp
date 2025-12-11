#pragma once
#include "engine/systems/ai/components/goap_action.hpp"

namespace tmt {

class KillPlayer : public GoapAction {
   public:
    KillPlayer() {
        preconditions["player_in_range"] = true;
        preconditions["player_alive"] = true;

        effects["player_alive"] = false;
        effects["player_in_range"] = false;
        effects["player_visible"] = false;

        cost = 2.f;
    }

    const char* get_name() const override { return "KillPlayer"; }

    void on_start(Entity /*agent*/, Registry& /*ecs*/) override {}

    bool is_done(Entity /*agent*/, Registry& /*ecs*/) const override {
        return true;  // Done instantly for testing purposes
    }

    void on_tick(Entity /*agent*/, Registry& /*ecs*/, float /*dt*/) override {}

    void on_finished(Entity /*agent*/, Registry& /*ecs*/) override {}

    void on_interrupt(Entity /*agent*/, Registry& /*ecs*/) override {}
};

}  // namespace tmt
