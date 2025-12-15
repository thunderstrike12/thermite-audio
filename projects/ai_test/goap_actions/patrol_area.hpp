#pragma once
#include "engine/systems/ai/components/goap_action.hpp"
#include <cstdlib>

namespace tmt {

class PatrolArea : public GoapAction {
    float timer = 0.f;

   public:
    PatrolArea() {
        preconditions["player_visible"] = false;
        effects["area_secure"] = true;
        cost = 1.f;
    }

    const char* get_name() const override { return "PatrolArea"; }

    void on_start(Entity /*agent*/, Registry& /*ecs*/) override { timer = 0.f; }

    void on_tick(Entity /*agent*/, Registry& /*ecs*/, float /*dt*/) override {}

    bool is_done(Entity /*agent*/, Registry& /*ecs*/) const override { return false; }

    void on_finished(Entity /*agent*/, Registry& /*ecs*/) override {}

    void on_interrupt(Entity agent, Registry& ecs) override {
        // reset initial world state
        auto& ws = ecs.get<WorldState>(agent);
        ws.facts[std::hash<std::string>()("player_visible")] = true;
        ws.facts[std::hash<std::string>()("player_in_range")] = false;
        ws.facts[std::hash<std::string>()("player_alive")] = true;
        ws.facts[std::hash<std::string>()("area_secure")] = false;
    }
};

}  // namespace tmt
