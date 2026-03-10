#pragma once

#include "engine/systems/ai/goap/components/goap_action.hpp"

namespace tmt {

class GoToTarget : public GoapAction {
   public:
    GoToTarget() {
        preconditions["can_move"] = true;
        effects["at_target"] = true;
        effects["can_move"] = false;
        cost = 2.f;
    }

    std::string get_id() const override { return "a_GoToTarget"; }

    void on_start(Entity agent) override;
    void on_tick(Entity /*agent*/, float /*dt*/) override {}  // Steering system handles movement automatically
    bool is_done(Entity agent) const override;
    void on_finished(Entity agent) override;
    void on_interrupt(Entity agent) override;

   private:
    glm::vec3 target = glm::vec3(10.f, 1.f, 20.f);
};

}  // namespace tmt
