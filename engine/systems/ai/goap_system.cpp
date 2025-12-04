#include "goap_system.hpp"
#include "core/logger.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"

namespace tmt {

void Goap::on_start() { Log::info("Goap on_start"); }

void Goap::on_update(const FrameData& time) {
    auto& registry = tmt::engine.ecs.get_registry();
    float dt = time.delta_time;

    for (auto [entity, goap, goal, world] : registry.view<GoapAgent, GoapGoal, WorldState>().each()) {
        process_agent(registry, entity, dt);
    }
}

void Goap::on_fixed_update(const FrameData&) {}

void Goap::on_end() { Log::info("Goap on_end"); }

// ------------------------------------------------------
// Main per-agent update
// ------------------------------------------------------
void Goap::process_agent(Registry& ecs, Entity entity, float dt) {
    auto& goap = ecs.get<GoapAgent>(entity);
    auto& goal = ecs.get<GoapGoal>(entity);
    auto& ws = ecs.get<WorldState>(entity);

    update_goal(ecs, entity, goap, goal, ws);
    if (goap.needs_replan) update_plan(ecs, entity, goap, goal, ws);
    update_action(ecs, entity, goap, dt);
}

// ------------------------------------------------------
// Step 1: goal assignment
// ------------------------------------------------------
void Goap::update_goal(Registry& ecs, Entity entity, GoapAgent& agent, GoapGoal& goal, WorldState& ws) {
    if (agent.has_goal) return;

    // TODO: goal selection logic here (look at world_state)

    agent.has_goal = true;
    agent.needs_replan = true;
}

// ------------------------------------------------------
// Step 2: planning
// ------------------------------------------------------
void Goap::update_plan(Registry& ecs, Entity entity, GoapAgent& agent, GoapGoal& goal, WorldState& ws) {
    agent.clear_plan();

    // TODO: implement A* planning using agent.actions

    if (!agent.plan.empty()) {
        agent.current_index = 0;
        agent.needs_replan = false;
    } else {
        // Could not create a valid plan
        Log::warn("GOAP: planning failed for agent {}", int(entity));
    }
}

// ------------------------------------------------------
// Step 3: action execution
// ------------------------------------------------------
void Goap::update_action(Registry& ecs, Entity entity, GoapAgent& agent, float dt) {
    GoapAction* action = agent.get_current_action();
    if (!action) return;  // no plan yet

    // TODO: logic for running the action
}

}  // namespace tmt
