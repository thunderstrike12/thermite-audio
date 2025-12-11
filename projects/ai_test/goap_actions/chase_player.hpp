#pragma once
// #include "engine/core/ecs.hpp"
#include "engine/systems/ai/components/goap_action.hpp"

namespace tmt {

class ChasePlayer : public GoapAction {
   public:
    ChasePlayer() {
        preconditions["player_visible"] = true;
        preconditions["player_alive"] = true;
        effects["player_in_range"] = true;
        cost = 4.f;
    }

    float time_chasing = 0.f;
    float chase_duration = 1.f;  // seconds

    const char* get_name() const override { return "ChasePlayer"; }

    void on_start(Entity /*agent*/, Registry& /*ecs*/) override { time_chasing = 0.f; }

    void on_tick(Entity /*agent*/, Registry& /*ecs*/, float dt) override { time_chasing += dt; }

    bool is_done(Entity /*agent*/, Registry& /*ecs*/) const override { return time_chasing >= chase_duration; }

    void on_finished(Entity /*agent*/, Registry& /*ecs*/) override {}

    void on_interrupt(Entity /*agent*/, Registry& /*ecs*/) override {}
};

}  // namespace tmt
