#pragma once

#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"

namespace game {

class PrepareExplode : public tmt::GoapAction {
   public:
    PrepareExplode() {
        preconditions["player_in_explosion_zone"] = true;
        effects["ready_to_explode"] = true;
        cost = 2.f;
    }

    std::string get_id() const override { return "a_PrepareExplode"; }

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
