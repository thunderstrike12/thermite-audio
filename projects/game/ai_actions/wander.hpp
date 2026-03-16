#pragma once
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

class Wander : public tmt::GoapAction {
   public:
    Wander() {
        preconditions["m_in_aggro_range"] = false;
        effects["m_wandering"] = true;
        cost = 2.f;
    }

    bool has_path = false;

    glm::vec3 wander_target;

    std::string get_id() const override { return "a_Wander"; }

    void on_start(tmt::Entity) override;
    void on_tick(tmt::Entity agent, float dt) override;
    bool is_done(tmt::Entity agent) const override;
    void on_finished(tmt::Entity) override {}
};
