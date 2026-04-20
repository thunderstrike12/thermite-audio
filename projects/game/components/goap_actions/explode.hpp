#pragma once

#include "engine/systems/ai/goap/components/goap_action.hpp"

namespace game {

class Explode : public tmt::GoapAction {
   public:
    Explode() {
        preconditions["s_ready_to_explode"] = true;
        effects["s_exploded"] = true;
        cost = 5.f;
    }

    std::string get_id() const override { return "a_Explode"; }

    void on_start(tmt::Entity agent) override;
    void on_tick(tmt::Entity agent, float /*dt*/) override;
    bool is_done(tmt::Entity agent) const override;
    void on_finished(tmt::Entity agent) override;
    void on_interrupt(tmt::Entity agent) override;

   private:
    tmt::Entity player_entity = entt::null;
    float live_charge_time = 0.0f;
    bool exploded = false;
};

}  // namespace game
