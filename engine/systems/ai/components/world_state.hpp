#pragma once
#include "engine/core/ecs.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace tmt {

// Represents a unique identifier for a world state fact.
struct FactId {
    uint32_t id;  // hash of the fact name

    FactId() : id(0u) {}
    FactId(const std::string& name) : id((uint32_t)std::hash<std::string> {}(name)) {}

    bool operator==(const FactId& other) const { return id == other.id; }
};

// Represents the value of a fact.
// Can be extended to support multiple types. Currently supports boolean, integer, and float.
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

// A simple pair of fact_id and its value.
// Used in preconditions, effects, and when applying changes to the world state.
struct FactPair {
    FactId id;
    FactValue value;
};

// Container for the agent's knowledge of the world state.
struct WorldState {
    std::unordered_map<uint32_t, FactValue> facts;

    // Apply a set of effects to the world state. Adds new facts if they don't exist, or updates existing ones.
    void apply(const std::vector<FactPair>& effects) {
        for (const auto& e : effects) {
            facts[e.id.id] = e.value;
        }
    }

    // Checks whether the world state satisfies a given set of conditions.
    // Returns true if all facts in `conditions` exist in the world state and have matching values.
    bool satisfies(const std::vector<FactPair>& conditions) const {
        for (const auto& cond : conditions) {
            auto it = facts.find(cond.id.id);
            if (it == facts.end()) return false;

            const auto& val = it->second;

            if (val.value_type == FactValue::Type::BOOL_TYPE && val.bool_val != cond.value.bool_val) return false;
        }
        return true;
    }

    // Helper to get a fact value safely.
    const FactValue* try_get(const FactId& id) const {
        auto it = facts.find(id.id);
        if (it != facts.end()) return &it->second;
        return nullptr;
    }
};

}  // namespace tmt
