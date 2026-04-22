#pragma once
#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

class Flee : public tmt::GoapAction {
   public:
    Flee() {
        preconditions["m_missiles_intact"] = false;
        preconditions["m_laser_intact"] = false;
        effects["m_stay_safe"] = true;
        cost = 1.f;
    }

    std::string get_id() const override { return "a_Flee"; };

    void on_start(tmt::Entity) override;
    void on_tick(tmt::Entity agent, float dt) override;
    bool is_done(tmt::Entity agent) const override;
    void on_finished(tmt::Entity) override {}
};
