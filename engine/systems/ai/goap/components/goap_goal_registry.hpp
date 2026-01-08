#pragma once
#include "goap_goal.hpp"
#include <unordered_map>
#include <memory>

namespace tmt {

/**
 * Class GoapGoalRegistry
 *
 * registry for all GOAP goals in the game.
 * Ensures that agents only use predefined goals from this registry.
 *   - Register new goals with a unique ID
 *   - Retrieve goals by ID
 *   - Provide read-only access to all registered goals
 *
 * Note:
 *   - Agents should not store their own goals; they reference the goals here.
 */
class GoapGoalRegistry {
   public:
    static GoapGoalRegistry& instance() {
        static GoapGoalRegistry inst;
        return inst;
    }

    void register_goal(const std::string& id, const GoapGoal& goal) { goals[id] = goal; }

    const GoapGoal* get(const std::string& id) const {
        auto it = goals.find(id);
        return (it != goals.end()) ? &it->second : nullptr;
    }

    const auto& get_all() const { return goals; }

   private:
    std::unordered_map<std::string, GoapGoal> goals;
};

}  // namespace tmt
