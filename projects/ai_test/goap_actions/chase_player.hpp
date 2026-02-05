#pragma once
// #include "engine/core/ecs.hpp"
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

namespace tmt {

class ChasePlayer : public GoapAction {
   public:
    ChasePlayer() {
        preconditions["player_visible"] = true;
        preconditions["player_alive"] = true;
        effects["player_in_range"] = true;
        cost = 4.f;
    }
    std::string get_id() const override { return "ChasePlayer"; };

    float time_chasing = 0.f;
    float chase_duration = 1.f;  // seconds

    void on_start(Entity /*agent*/) override { time_chasing = 0.f; }

    void on_tick(Entity /*agent*/, float dt) override { time_chasing += dt; }

    bool is_done(Entity /*agent*/) const override { return time_chasing >= chase_duration; }

    void on_finished(Entity /*agent*/) override {}

    void on_interrupt(Entity /*agent*/) override {}
};

}  // namespace tmt
