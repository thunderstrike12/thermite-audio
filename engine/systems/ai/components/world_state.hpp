#pragma once
#include "engine/core/ecs.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace tmt {

/**
 * Class FactRegistry
 *
 *   - Allows editor and debug systems (ImGui, logging, inspectors)
 *     to display fact names instead of numeric hashes.
 *   - Keeps the runtime GOAP planner fast by still using hashed IDs.
 *   - WorldState and planner logic never depend on strings.
 *
 * Note:
 *   Hash collisions are possible but very unlikely
 *   for short, well-defined gameplay fact names.
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
        uint32_t id = std::hash<std::string> {}(name);
        id_to_name[id] = name;
        return id;
    }

    /**
     * Retrieves the human-readable name for a fact ID.
     * Returns "<unknown>" if the ID was never registered.
     */
    const std::string& get_name(uint32_t id) const {
        static const std::string unknown = "<unknown>";
        auto it = id_to_name.find(id);
        return it != id_to_name.end() ? it->second : unknown;
    }

   private:
    std::unordered_map<uint32_t, std::string> id_to_name;
};

/**
 * Struct FactId
 * Represents a unique identifier for a world-state fact.
 *
 * Internally stores a hashed string ID for fast comparisons.
 * When constructed from a string, the name is automatically
 * registered with the FactRegistry for debug and editor use.
 */
struct FactId {
    uint32_t id;

    FactId() : id(0) {}

    explicit FactId(const std::string& name) { id = FactRegistry::instance().register_fact(name); }

    bool operator==(const FactId& other) const { return id == other.id; }
};

/**
 * Struct FactValue
 * Represents the stored type and data for a world state fact.
 *
 * Supports:
 *   - bool
 *   - int
 *   - float
 *
 * Currently only bool types are supported by GoapAction::check_preconditions.
 * Could be extended to support more complex types.
 */
struct FactValue {
    enum class Type { BOOL_TYPE, INT_TYPE, FLOAT_TYPE } value_type;

    union {
        bool bool_val;
        int int_val;
        float float_val;
    };

    FactValue() : value_type(Type::BOOL_TYPE), bool_val(false) {}
    FactValue(bool b) : value_type(Type::BOOL_TYPE), bool_val(b) {}
    FactValue(int i) : value_type(Type::INT_TYPE), int_val(i) {}
    FactValue(float f) : value_type(Type::FLOAT_TYPE), float_val(f) {}
};

/**
 * Struct FactPair
 * A single fact assignment (ID + value).
 *
 * Used for preconditions, effects and world state application.
 */
struct FactPair {
    FactId id;
    FactValue value;
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
    std::unordered_map<uint32_t, FactValue> facts;

    /**
     * Applies a collection of effects to the world state.
     * Param: effects, a List of facts to modify.
     */
    void apply(const std::vector<FactPair>& effects) {
        for (const auto& e : effects) {
            facts[e.id.id] = e.value;
        }
    }

    /**
     * Checks whether the world state satisfies a given set of conditions.
     * Returns true if all conditions are satisfied.
     * Param: conditions, A list of required facts.
     */
    bool satisfies(const std::vector<FactPair>& conditions) const {
        for (const auto& cond : conditions) {
            auto it = facts.find(cond.id.id);
            if (it == facts.end()) return false;

            const auto& val = it->second;

            if (val.value_type == FactValue::Type::BOOL_TYPE && val.bool_val != cond.value.bool_val) return false;
        }
        return true;
    }

    /**
     * Retrieves a fact value if it exists.
     * Returns pointer to FactValue or nullptr.
     */
    const FactValue* try_get(const FactId& id) const {
        auto it = facts.find(id.id);
        if (it != facts.end()) return &it->second;
        return nullptr;
    }
};

}  // namespace tmt
