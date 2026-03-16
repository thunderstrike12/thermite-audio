#pragma once

#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"

namespace game {

class SteerToPlayer : public tmt::GoapAction {
   public:
    SteerToPlayer() {
        preconditions["player_in_range"] = true;
        effects["player_in_explosion_zone"] = true;
        cost = 2.f;

        wants_fixed_update = true;
    }

    std::string get_id() const override { return "a_SteerToPlayer"; }

    void on_start(tmt::Entity agent) override;
    void on_tick(tmt::Entity agent, float /*dt*/) override {}
    void on_fixed_tick(tmt::Entity agent, float /*dt*/) override;
    bool is_done(tmt::Entity agent) const override;
    void on_finished(tmt::Entity agent) override;
    void on_interrupt(tmt::Entity agent) override;

   private:
    tmt::Entity player_entity = entt::null;
    //bool done = false;
};

}  // namespace game
