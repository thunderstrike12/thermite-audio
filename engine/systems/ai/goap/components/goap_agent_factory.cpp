#include "goap_agent_factory.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/logger.hpp"
#include "core/components/transform.hpp"

#include "world_state.hpp"
#include "goap_agent.hpp"
#include "goap_action_registry.hpp"
#include "goap_goal_registry.hpp"
#include "goap_agent_factory.hpp"
#include "goap_agent_type_registry.hpp"

#include "../goap_system.hpp"

namespace tmt {

void GoapAgentFactory::spawn_agent_from_type(const std::string& type_id, Entity e) {
    auto& ecs = engine.ecs;

    // --- Get GOAP system ---
    Goap* goap = ecs.systems.try_get<Goap>();
    if (!goap) {
        Log::warn("GOAP system not active, cannot spawn agent '{}'", type_id);
        return;
    }

    auto& type_registry = goap->agent_types();
    auto& action_registry = goap->actions();
    auto& goal_registry = goap->goals();

    // --- Look up the agent type ---
    const GoapAgentType* type = type_registry.get(type_id);
    if (!type) {
        Log::warn("Unknown agent type: {}", type_id);
        return;
    }

    // --- Required components ---
    ecs.get_component<Transform>(e);  // ensure exists
    // --- Create the entity ---
    auto& agent = ecs.add_component<GoapAgent>(e);
    auto& ws = ecs.add_component<WorldState>(e);

    // --- Assign actions ---
    for (const auto& action_id : type->action_ids) {
        if (GoapAction* action = action_registry.get(action_id)) {
            agent.available_actions.push_back(action);
        } else {
            Log::warn("GOAP Agent '{}': unknown action '{}'", type_id, action_id);
        }
    }

    // --- Assign goals ---
    for (const auto& goal_id : type->goal_ids) {
        if (const GoapGoal* goal = goal_registry.get(goal_id)) {
            agent.available_goals.push_back(*goal);
        } else {
            Log::warn("GOAP Agent '{}': unknown goal '{}'", type_id, goal_id);
        }
    }

    // --- Initialize world state ---
    ws.facts.clear();
    for (const auto& [fact_name, value] : type->default_world_state) {
        // Convert string to uint32 hash
        // uint32_t id = static_cast<uint32_t>(std::hash<std::string> {}(fact_name));
        uint32_t id = tmt::FactId(fact_name).id;
        ws.facts[id] = value;
    }

    // Force initial planning
    agent.needs_replan = true;
}

}  // namespace tmt
