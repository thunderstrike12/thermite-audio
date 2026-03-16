#pragma once

#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"

namespace game {

class WanderSteering : public tmt::GoapAction {
   public:
    WanderSteering() {
        preconditions["player_in_range"] = false;
        effects["wandering"] = true;
        cost = 1.f;

        wants_fixed_update = true;
    }

    std::string get_id() const override { return "a_WanderSteering"; }

    void on_start(tmt::Entity agent) override;
    void on_tick(tmt::Entity agent, float /*dt*/) override {}
    void on_fixed_tick(tmt::Entity agent, float /*dt*/) override;
    bool is_done(tmt::Entity agent) const override;
    void on_finished(tmt::Entity agent) override;
    void on_interrupt(tmt::Entity agent) override;

   private:
    tmt::Entity player_entity = entt::null;
};

}  // namespace game
