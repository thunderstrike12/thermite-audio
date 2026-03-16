#pragma once

#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"

namespace game {

class Explode : public tmt::GoapAction {
   public:
    Explode() {
        preconditions["ready_to_explode"] = true;
        effects["exploded"] = true;
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
};

}  // namespace game
