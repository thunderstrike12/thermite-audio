#pragma once
#include "engine/core/ecs.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace tmt {

/**
 * Struct FactId
 * Represents a unique identifier for a world-state variable.
 *
 * FactId uses a hash of a string.
 */
struct FactId {
    uint32_t id;  // hash of the fact name

    FactId() : id(0) {}
    FactId(const std::string& name) : id(std::hash<std::string> {}(name)) {}

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
