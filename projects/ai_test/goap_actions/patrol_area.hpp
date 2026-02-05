#pragma once
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"
#include "engine/engine.hpp"

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

    std::string get_id() const override { return "PatrolArea"; };

    void on_start(Entity /*agent*/) override { timer = 0.f; }

    void on_tick(Entity /*agent*/, float /*dt*/) override {}

    bool is_done(Entity /*agent*/) const override { return false; }

    void on_finished(Entity /*agent*/) override {}

    void on_interrupt(Entity agent) override {
        // reset initial world state
        auto& ws = engine.ecs.get_registry().get<WorldState>(agent);
        ws.facts[std::hash<std::string>()("player_visible")] = true;
        ws.facts[std::hash<std::string>()("player_in_range")] = false;
        ws.facts[std::hash<std::string>()("player_alive")] = true;
        ws.facts[std::hash<std::string>()("area_secure")] = false;
    }
};

}  // namespace tmt
