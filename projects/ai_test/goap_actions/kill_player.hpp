#pragma once
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

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

    std::string get_id() const override { return "a_KillPlayer"; };

    float time_killing = 0.f;
    float kill_duration = 0.5f;  // seconds

    void on_start(Entity /*agent*/) override { time_killing = 0.f; }

    void on_tick(Entity /*agent*/, float dt) override { time_killing += dt; }

    bool is_done(Entity /*agent*/) const override { return time_killing >= kill_duration; }

    void on_finished(Entity /*agent*/) override {}

    void on_interrupt(Entity /*agent*/) override {}
};

}  // namespace tmt
