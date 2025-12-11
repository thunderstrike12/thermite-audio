#pragma once
#include "engine/core/ecs.hpp"
#include "world_state.hpp"

/**
 * Class GoapAction
 * Base class for all GOAP actions. Inherit this to define behavior.
 *
 * GOAP actions consist of:
 *   - Preconditions   (must be true to consider action)
 *   - Effects         (world modifications)
 *   - Cost            (the higher the cost, the less likely the GOAP planner will pick it)
 *   - Execution logic (start, tick, finish, interrupt)
 *
 * The planner uses only the preconditions, effects, and cost.
 * The GOAP system uses the execution callbacks.
 */

namespace tmt {

class GoapAction {
   public:
    GoapAction() = default;
    virtual ~GoapAction() = default;

    // These are used by the planner
    std::unordered_map<std::string, bool> preconditions;
    std::unordered_map<std::string, bool> effects;
    float cost = 1.f;

    // Used during action execution
    bool is_running = false;

    // readable name
    virtual const char* get_name() const = 0;

    /**
     * Checks if world state satisfies the action's preconditions.
     * Returns true if all preconditions match.
     */
    virtual bool check_preconditions(const WorldState& ws) const;

    /**
     * Applies action effects to the world state.
     */
    virtual void apply_effects(WorldState& ws) const;  // calls apply() from ws

    /**
     * Execution Callbacks
     *
     * - start:      called once when the action begins.
     * - tick:       called every frame while action is running.
     * - finished:   called only when the action finished normally.
     */
    virtual void on_start(Entity /*agent*/, Registry& /*ecs*/) {}
    virtual void on_tick(Entity /*agent*/, Registry& /*ecs*/, float /*dt*/) {}
    virtual void on_finished(Entity /*agent*/, Registry& /*ecs*/) {}

    /**
     * Called every tick to determine whether the action is done,
     * but only while the action is actively running.
     * Returns true to advance to the next action, per actions this will be different.
     */
    virtual bool is_done(Entity /*agent*/, Registry& /*ecs*/) const { return true; }

    /**
     * Called when an action is externally interrupted.
     */
    virtual void on_interrupt(Entity /*agent*/, Registry& /*ecs*/) {}
};

}  // namespace tmt
