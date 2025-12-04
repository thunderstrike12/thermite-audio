#pragma once
#include "core/system.hpp"
#include "components/goap_goal.hpp"
#include "components/world_state.hpp"
#include "components/goap_agent.hpp"

namespace tmt {

class Goap : public ISystem {
   public:
    Goap() = default;

    // Inherited via ISystem
    std::string get_name() override { return "Goap System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

   private:
    void process_agent(Registry& ecs, Entity entity, float dt);

    // Step 1: Assign goal if needed
    void update_goal(Registry& ecs, Entity entity, GoapAgent& agent, GoapGoal& goal, WorldState& ws);

    // Step 2: Build plan if needed
    void update_plan(Registry& ecs, Entity entity, GoapAgent& agent, GoapGoal& goal, WorldState& ws);

    // Step 3: Execute current action
    void update_action(Registry& ecs, Entity entity, GoapAgent& agent, float dt);
};

}  // namespace tmt
