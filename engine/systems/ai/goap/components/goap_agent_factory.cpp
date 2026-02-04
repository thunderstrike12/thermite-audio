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

namespace tmt {

void GoapAgentFactory::spawn_agent_from_type(const std::string& type_id, const Entity e) {
    auto& ecs = engine.ecs;

    // --- Look up the agent type ---
    auto* type = GoapAgentTypeRegistry::instance().get(type_id);
    if (!type) {
        Log::warn("Unknown agent type: {}", type_id);
        return;
    }

    // --- Create the entity ---
    auto& agent = ecs.add_component<GoapAgent>(e);
    auto& ws = ecs.add_component<WorldState>(e);

    // --- Assign actions ---
    for (auto& id : type->action_ids) {
        if (auto* a = GoapActionRegistry::instance().get(id)) {
            agent.available_actions.push_back(a);
        }
    }

    // --- Assign goals ---
    for (auto& gid : type->goal_ids) {
        if (auto* g = GoapGoalRegistry::instance().get(gid)) {
            agent.available_goals.push_back(*g);
        }
    }

    // --- Initialize world state ---
    ws.facts = type->default_world_state;
}

}  // namespace tmt
