#pragma once
#include "goap_action.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace tmt {

/**
 * Class GoapActionRegistry
 *
 * Stores all GOAP actions in the game.
 *  - Register new GoapAction instances.
 *  - Provide access to actions by ID.
 */
class GoapActionRegistry {
   public:
    /*static GoapActionRegistry& instance() {
        static GoapActionRegistry inst;
        return inst;
    }*/

    // Register a new action.
    void register_action(std::unique_ptr<GoapAction> action) { actions[action->get_id()] = std::move(action); }

    // Get an action by its ID.
    GoapAction* get(const std::string& id) {
        auto it = actions.find(id);
        return (it != actions.end()) ? it->second.get() : nullptr;
    }

    // Get all registered actions.
    const auto& get_all() const { return actions; }

   private:
    // Internal storage of all registered actions
    std::unordered_map<std::string, std::unique_ptr<GoapAction>> actions;
};

}  // namespace tmt
