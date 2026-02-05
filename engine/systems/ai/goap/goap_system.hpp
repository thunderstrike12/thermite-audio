#pragma once
#include "engine/core/system.hpp"
#include "components/goap_goal.hpp"
#include "components/world_state.hpp"
#include "components/goap_agent.hpp"
#include "components/goap_action_editor_data.hpp"
#include "components/goap_action_overrides.hpp"
#include "components/goap_agent_type_registry.hpp"
#include "components/goap_action_registry.hpp"
#include "components/goap_goal_registry.hpp"
#include "components/goap_action_overrides.hpp"

/**
 * Class Goap
 * Main GOAP (Goal-Oriented Action Planning) system.
 *
 * This system:
 *  - Selects goals for agents based on world state.
 *  - Builds plans using GOAP A*.
 *  - Executes actions, handles interruptions, and detects completion.
 *
 * The system runs automatically inside the engine's ECS update loop.
 *
 * Logging information is optional, can set show_logging to true.
 * Warnings will always be shown, like planning fails etc.
 */

namespace tmt {

class Goap : public ISystem {
   public:
    Goap() = default;

    // Publicly accesable registries
    GoapActionRegistry& actions() { return action_registry; }
    GoapActionOverrides& overrides() { return action_overrides; }
    GoapGoalRegistry& goals() { return goal_registry; }
    GoapAgentTypeRegistry& agent_types() { return agent_type_registry; }

    // Inherited via ISystem
    std::string get_name() override { return "Goap System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

   private:
    /**
     * Executes the full GOAP update for a single agent.
     *
     * Steps:
     *  1. Evaluate and assign goals.
     *  2. Replan if required.
     *  3. Tick and manage action execution.
     */
    void process_agent(Entity entity, WorldState& ws, float dt);

    // Selects or updates the active goal for an agent.
    void update_goal(Entity entity, GoapAgent& agent, WorldState& ws);

    // Builds a plan toward the current goal (A*).
    void update_plan(Entity entity, GoapAgent& agent, WorldState& ws);

    // Starts & validates the current action, actual running of the action happens
    // in on_update() and on_fixed_update().
    void update_action(Entity entity, GoapAgent& agent, WorldState& ws);

    // Registries & overrides
    GoapActionRegistry action_registry;
    GoapActionOverrides action_overrides;
    GoapGoalRegistry goal_registry;
    GoapAgentTypeRegistry agent_type_registry;

    // Set to true if you want more logging to see whats happening internally
    bool show_logging = false;
};

/**
 * Helper struct for live edits in the editor, actions keep their origiona values,
 * and only this takes care of the updates.
 */
struct EffectiveGoapAction {
    float cost;
    std::unordered_map<std::string, bool> preconditions;
    std::unordered_map<std::string, bool> effects;
};

/**
 * Build an effective action that merges base GoapAction with editor overrides.
 */
EffectiveGoapAction build_effective_action(const GoapAction& base, const GoapActionEditorData* override);

}  // namespace tmt
