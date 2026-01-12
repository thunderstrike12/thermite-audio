#pragma once
#include "engine/core/ecs.hpp"
#include <memory>

namespace tmt {

/**
 * Class GoapAgentFactory
 *
 * Responsible for spawning GOAP agents from predefined agent types.
 *   - Agent types must be registered in GoapAgentTypeRegistry before calling spawn.
 *   - Automatically assigns:
 *       - Actions from GoapActionRegistry
 *       - Goals from GoapGoalRegistry
 *       - Default initial world-state
 *   - Returns an entity with a Transform, GoapAgent, and WorldState component
 */
class GoapAgentFactory {
   public:
    /**
     * Spawns a new GOAP agent based on a registered agent type
     * Retrieves the agent type from GoapAgentTypeRegistry
     */

    static void spawn_agent_from_type(const std::string& type_id, const Entity entity);
};

}  // namespace tmt
