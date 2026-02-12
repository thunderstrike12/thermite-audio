#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

#include "world_state.hpp"

#include "engine/core/reflection.hpp"

namespace tmt {

/**
 * Struct GoapAgentType
 *
 * Defines a template for a type of GOAP agent.
 * Each agent type specifies:
 *   - Which actions they have
 *   - Which goals they have
 *   - Its default initial world state
 *
 * Instances of this type are used by GoapAgentFactory to spawn actual agents.
 */
struct GoapAgentType {
    // Unique identifier for this agent type
    std::string id;

    // List of action IDs that agents of this type can execute
    std::vector<std::string> action_ids;

    // List of goal IDs that agents of this type can have
    std::vector<std::string> goal_ids;

    // Default initial world state for agents of this type
    // Change from uint32_t to string here for editor and serilisation
    std::unordered_map<std::string, FactValue> default_world_state;
};

/**
 * Class GoapAgentTypeRegistry
 *
 * Registry for all GOAP agent types.
 * Makes sure agents are spawned consistently with predefined actions, goals, and default state.
 *
 * Responsibilities:
 *   - Register new agent types
 *   - Retrieve agent types by ID
 *   - Provide read-only access to all registered types
 *
 * Usage:
 *   - Define agent types at game startup
 *   - Spawn agents using GoapAgentFactory with a registered type ID
 */
class GoapAgentTypeRegistry {
   public:
    // Registers a new agent type
    void register_type(const GoapAgentType& type) { types[type.id] = type; }

    // Retrieve an agent type by its ID
    const GoapAgentType* get(const std::string& id) const {
        auto it = types.find(id);
        return (it != types.end()) ? &it->second : nullptr;
    }

    // Get all registered agent types
    const auto& get_all() const { return types; }

    // Remove an agent type by ID
    void remove_type(const std::string& id) { types.erase(id); }

    // Internal storage mapping type ID -> GoapAgentType
    std::unordered_map<std::string, GoapAgentType> types;

    void load();
    void save() const;

   private:
};

}  // namespace tmt

TMT_OBJECT(tmt::GoapAgentType, (id, action_ids, goal_ids, default_world_state));
TMT_OBJECT(tmt::GoapAgentTypeRegistry, (types));
