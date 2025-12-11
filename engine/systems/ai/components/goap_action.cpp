#include "goap_action.hpp"
#include "core/logger.hpp"

namespace tmt {

/**
 * Checks whether all of this action's preconditions are satisfied by the given WorldState.
 *
 * For each precondition:
 *   - Convert the string key into a FactId
 *   - Check if the fact exists in the world state
 *   - Check that the fact is a BOOL fact
 *   - Check that it matches the expected boolean value
 *
 * If any precondition is missing, mismatched, or wrong type,
 * the action cannot be used by the planner.
 *
 * Return true if all preconditions matched.
 * Return false if at least one precondition failed.
 */
bool GoapAction::check_preconditions(const WorldState& ws) const {
    for (const auto& [key, want] : preconditions) {
        // Convert string key into hashed FactId
        FactId fid(key);

        // Look up fact in the world state
        const FactValue* fv = ws.try_get(fid);
        if (!fv) {
            // Fact is missing -> precondition fails
            return false;
        }

        // Preconditions currently only support boolean facts.
        if (fv->value_type != FactValue::Type::BOOL_TYPE) {
            // Wrong type -> treat as not satisfied
            return false;
        }

        // Compare world value with expected value
        if (fv->bool_val != want) {
            // Wrong value -> precondition not met
            return false;
        }
    }

    return true;
}

/**
 * Utility function to print all facts in a WorldState.
 *
 * This is used only for debugging so you can inspect how actions
 * modify the agent’s perceived world after each step.
 * You can't see actual names, just the "number" but it is useful for debugging.
 */
void print_world_state(const WorldState& ws) {
    Log::info("WorldState:");
    for (const auto& [id, val] : ws.facts) {
        std::string val_str;
        switch (val.value_type) {
            case FactValue::Type::BOOL_TYPE:
                val_str = val.bool_val ? "true" : "false";
                break;
            case FactValue::Type::INT_TYPE:
                val_str = std::to_string(val.int_val);
                break;
            case FactValue::Type::FLOAT_TYPE:
                val_str = std::to_string(val.float_val);
                break;
        }
        Log::info("  FactID {} = {}", id, val_str);
    }
}

/**
 * Applies this action's effects to the given WorldState.
 *
 * The method converts them into a vector<FactPair>, then calls:
 * ws.apply(effects_vec)
 *
 * This updates or inserts facts in the world state.
 * The updated world state can be printed for debugging.
 */
void GoapAction::apply_effects(WorldState& ws) const {
    std::vector<FactPair> effects_vec;
    effects_vec.reserve(effects.size());

    for (const auto& [key, val] : effects) {
        FactPair fp;
        fp.id = FactId(key);
        fp.value = FactValue(val);
        effects_vec.push_back(fp);
    }

    // Apply changes to the provided world state
    ws.apply(effects_vec);
    // print_world_state(ws);
}

}  // namespace tmt
