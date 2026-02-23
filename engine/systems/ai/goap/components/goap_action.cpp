#include "goap_action.hpp"
#include "core/logger.hpp"

namespace tmt {

/**
 * Checks whether all preconditions of this action are satisfied by the given WorldState.
 *
 * For each precondition:
 *   - Convert the string key into a FactId
 *   - Look up the fact in the WorldState
 *   - Compare the stored boolean value with the expected value
 *
 * Returns:
 *   - true if all preconditions exist in the world state and match
 *   - false if any precondition is missing or does not match
 *
 * Notes:
 *   - Only boolean facts are supported.
 *   - WorldState and preconditions are keyed by hashed FactId for efficiency.
 *   - If any precondition is missing, mismatched, or wrong type,
 *   the action cannot be used by the planner.
 */
bool GoapAction::check_preconditions(const WorldState& ws) const {
    for (const auto& [key, want] : preconditions) {
        FactId fid(key);

        const bool* have = ws.try_get(fid);
        if (!have) {
            // Fact missing
            return false;
        }

        if (*have != want) {
            // Wrong value
            return false;
        }
    }
    return true;
}

/**
 * Utility function for debugging: prints all facts in a WorldState.
 *
 * Shows each FactId and its boolean value.
 * Useful for inspecting the agent's perceived world at runtime.
 *
 * Only for logging/debugging purposes; does not modify the world state.
 */
void print_world_state(const WorldState& ws) {
    Log::info("WorldState:");
    for (const auto& [id, val] : ws.facts) {
        Log::info("  FactID {} = {}", id, val ? "true" : "false");
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
/**
 * Applies this action's effects to a given WorldState.
 *
 * - Convert the action's effect map (string -> bool) into a vector of FactPairs
 * - Call WorldState::apply to update or insert each fact
 *
 * Notes:
 *   - WorldState is updated immediately and can be inspected with print_world_state.
 */
void GoapAction::apply_effects(WorldState& ws) const {
    std::vector<FactPair> effects_vec;
    effects_vec.reserve(effects.size());

    for (const auto& [key, val] : effects) {
        effects_vec.push_back({ FactId(key), val });
    }

    ws.apply(effects_vec);
}

}  // namespace tmt
