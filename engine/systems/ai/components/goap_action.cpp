#include "goap_action.hpp"

namespace tmt {

// Default implementation: check that every precondition in the action
// is present in the world state and has the same boolean value.
// Returns false if a fact is missing or mismatched.
bool GoapAction::check_preconditions(const WorldState& ws) const {
    for (const auto& [key, want] : preconditions) {
        FactId fid(key);
        const FactValue* fv = ws.try_get(fid);
        if (!fv) {
            // fact missing
            return false;
        }

        if (fv->value_type != FactValue::Type::BOOL_TYPE) {
            // mismatch type -> treat as not satisfied
            return false;
        }

        if (fv->bool_val != want) {
            return false;
        }
    }

    return true;
}

// Default implementation: apply this action's effects to the provided world state.
// Converts unordered_map<string,bool> -> vector<FactPair> then calls WorldState::apply().
void GoapAction::apply_effects(WorldState& ws) const {
    std::vector<FactPair> effects_vec;
    effects_vec.reserve(effects.size());

    for (const auto& [key, val] : effects) {
        FactPair fp;
        fp.id = FactId(key);
        fp.value = FactValue(val);
        effects_vec.push_back(fp);
    }

    ws.apply(effects_vec);
}

void GoapAction::on_start(entt::entity, entt::registry&) {}
void GoapAction::on_update(entt::entity, entt::registry&, float) {}

}  // namespace tmt
