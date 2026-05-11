#pragma once
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

class Stomp : public tmt::GoapAction {
   public:
    Stomp() {
        preconditions["m_in_stomp_range"] = true;
        preconditions["m_stomp_ready"] = true;
        effects["m_kill_player"] = true;
        cost = 2.f;
    }

    float time = 0.0f;

    std::string get_id() const override { return "a_Stomp"; }

    void on_start(tmt::Entity) override;
    void on_tick(tmt::Entity agent, float dt) override;
    bool is_done(tmt::Entity agent) const override;
    void on_finished(tmt::Entity) override {}
    void on_interrupt(tmt::Entity) override;
};
