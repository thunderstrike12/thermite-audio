#pragma once
#include "engine/core/ecs.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace tmt {

/**
 * Class FactRegistry
 *
 * Central registry that maps hashed fact IDs to readable names.
 *
 * Purpose:
 *   - Allows editor and debug systems (ImGui, logging, inspectors)
 *     to display fact names instead of numeric hashes.
 *   - Keeps the runtime GOAP planner fast by still using hashed IDs.
 *   - WorldState and planner logic never depend on strings.
 *
 * Note:
 *   - Hash collisions are theoretically possible but unlikely
 *     when using short gameplay fact names.
 */
class FactRegistry {
   public:
    /**
     * Returns the global FactRegistry instance.
     */
    static FactRegistry& instance() {
        static FactRegistry inst;
        return inst;
    }

    /**
     * Registers a fact name and returns its hashed ID.
     * If the fact already exists, the existing mapping is reused.
     */
    uint32_t register_fact(const std::string& name) {
        uint32_t id = (uint32_t)std::hash<std::string> {}(name);
        id_to_name[id] = name;
        return id;
    }

    /**
     * Retrieves the human-readable name for a fact ID.
     * Returns "<unknown>" if the ID was never registered.
     */
    const std::string& get_name(uint32_t id) const {
        static const std::string UNKNOWN = "<unknown>";
        auto it = id_to_name.find(id);
        return it != id_to_name.end() ? it->second : UNKNOWN;
    }

   private:
    std::unordered_map<uint32_t, std::string> id_to_name;
};

/**
 * Struct FactId
 * Represents a unique identifier for a world-state fact.
 *
 * When constructed from a string:
 *   - The string is hashed.
 *   - The name is automatically registered with FactRegistry
 *     for editor/debug visibility.
 *
 * FactId is used everywhere in GOAP logic instead of strings
 * to ensure fast comparisons and minimal memory usage.
 */
struct FactId {
    uint32_t id;

    FactId() : id(0) {}

    explicit FactId(const std::string& name) { id = FactRegistry::instance().register_fact(name); }

    bool operator==(const FactId& other) const { return id == other.id; }
};

/**
 * Struct FactPair
 * A single fact assignment (ID + value).
 *
 * Used for preconditions, effects and world state application.
 * Example:
 *   { FactId("HasWeapon"), true }
 */
struct FactPair {
    FactId id;
    bool value;
};

/**
 * Struct WorldState
 * A container storing an agent's local perception of the world.
 *
 * WorldState drives:
 *   - goal relevance checks,
 *   - action preconditions,
 *   - planning,
 *   - dynamic reaction and interrupts.
 */
struct WorldState {
    std::unordered_map<uint32_t, bool> facts;

    /**
     * Applies a list of fact assignments (effects) to the world state.
     *
     * If a fact does not exist yet, it will be created.
     * Existing facts are overwritten.
     */
    void apply(const std::vector<FactPair>& effects) {
        for (const auto& e : effects) {
            facts[e.id.id] = e.value;
        }
    }

    /**
     * Checks whether all provided conditions are satisfied.
     *
     * Returns true only if:
     *   - Every condition exists in the world state
     *   - Every stored value matches the requested value
     */
    bool satisfies(const std::vector<FactPair>& conditions) const {
        for (const auto& cond : conditions) {
            auto it = facts.find(cond.id.id);
            if (it == facts.end()) return false;
            if (it->second != cond.value) return false;
        }
        return true;
    }

    /**
     * Attempts to retrieve a fact value.
     *
     * Returns:
     *   - Pointer to bool if the fact exists
     *   - nullptr if the fact is not present
     *
     * This does not create the fact.
     */
    const bool* try_get(const FactId& id) const {
        auto it = facts.find(id.id);
        return it != facts.end() ? &it->second : nullptr;
    }
};

}  // namespace tmt

TMT_OBJECT(tmt::FactId, (id));
TMT_OBJECT(tmt::FactPair, (id, value));
TMT_COMPONENT(tmt::WorldState, "WorldState", (facts));
