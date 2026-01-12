#pragma once
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

class Wander : public tmt::GoapAction {
   public:
    Wander() {
        preconditions["player_in_range"] = false;
        effects["player_in_range"] = true;
        cost = 2.f;
    }

    tmt::Entity player = entt::null;

    bool done_walking = false;
    bool has_path = false;

    glm::vec3 wander_target;

    std::string get_id() const override { return "Wander"; }

    void on_start(tmt::Entity, tmt::Registry&) override;
    void on_tick(tmt::Entity agent, tmt::Registry& ecs, float dt) override;
    bool is_done(tmt::Entity agent, tmt::Registry& ecs) const override;
    void on_finished(tmt::Entity, tmt::Registry&) override {}
};
