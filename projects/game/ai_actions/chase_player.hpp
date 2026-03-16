#pragma once
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

class ChasePlayer : public tmt::GoapAction {
   public:
    ChasePlayer() {
        preconditions["m_in_aggro_range"] = true;
        effects["m_in_laser_range"] = true;
        cost = 1.f;
    }

    tmt::Entity player = entt::null;

    std::string get_id() const override { return "a_ChasePlayer"; };

    void on_start(tmt::Entity) override;
    void on_tick(tmt::Entity agent, float dt) override;
    bool is_done(tmt::Entity agent) const override;
    void on_finished(tmt::Entity) override {}
};
