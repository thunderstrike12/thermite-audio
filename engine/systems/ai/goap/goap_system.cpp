#include "goap_system.hpp"
#include "core/logger.hpp"

#include "components/goap_agent_factory.hpp"
#include "components/goap_agent_type_ref.hpp"

#include "systems/ai/steering/components/steering_agent.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"

#include <queue>
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace tmt {

void Goap::on_start() {
    Log::info("Goap on_start");
    agent_types().load();

    auto& registry = engine.ecs.get_registry();

    // Build runtime agents from serialized scene data
    /*for (auto entity : engine.ecs.view<GoapAgentTypeRef>()) {
        auto& type_ref = registry.get<GoapAgentTypeRef>(entity);

        // Avoid double building if already exists
        if (registry.any_of<GoapAgent>(entity)) continue;

        GoapAgentFactory::spawn_agent_from_type(type_ref.type_id, entity);
    }*/

    for (auto entity : engine.ecs.view<GoapAgentTypeRef>()) {
        auto& type_ref = registry.get<GoapAgentTypeRef>(entity);

        if (!registry.any_of<GoapAgent>(entity)) {
            GoapAgentFactory::spawn_agent_from_type(type_ref.type_id, entity);
        }

        registry.remove<GoapAgentTypeRef>(entity);
    }
}

/**
 * Main update loop called every frame.
 *
 * Iterates over all entities that have both:
 *   - GoapAgent
 *   - WorldState
 *
 * For each agent, run the GOAP lifecycle:
 *    1. Select a goal (if needed)
 *    2. Plan (if needed)
 *    3. Execute action(s)
 */
void Goap::on_update(const FrameData& time) {
    auto& registry = engine.ecs.get_registry();

    for (auto entity : engine.ecs.view<GoapAgentTypeRef>()) {
        if (!registry.any_of<GoapAgent>(entity)) {
            auto& type_ref = registry.get<GoapAgentTypeRef>(entity);
            GoapAgentFactory::spawn_agent_from_type(type_ref.type_id, entity);
        }

        registry.remove<GoapAgentTypeRef>(entity);
    }

    for (auto [entity, agent, world] : engine.ecs.view<GoapAgent, WorldState>().each()) {
        process_agent(entity, world, time.delta_time);

        if (agent.current_action && agent.current_action->is_running && agent.current_action->wants_fixed_update == false) {
            agent.current_action->on_tick(entity, time.delta_time);
        }
    }
}

void Goap::on_fixed_update(const FrameData& time) {
    for (auto [entity, agent] : engine.ecs.view<GoapAgent>().each()) {
        if (agent.current_action && agent.current_action->is_running && agent.current_action->wants_fixed_update == true) {
            agent.current_action->on_fixed_tick(entity, time.delta_time);
        }
    }
}

void Goap::on_end() {
    Log::info("Goap on_end");
}

// ------------------------------------------------------
// Main per-agent update
// ------------------------------------------------------
/**
 * Executes a full GOAP cycle for a single agent:
 *
 *  1. update_goal()   : choose what the agent wants now
 *  2. update_plan()   : generate a new plan when needed
 *  3. update_action() : tick the action currently running
 */
/*void Goap::process_agent(Entity entity, WorldState& ws, float) {
    auto& registry = engine.ecs.get_registry();
    auto& agent = registry.get<GoapAgent>(entity);

    // Only try planning if we need to
    if (agent.needs_replan || agent.plan.empty()) {
        try_plan_goals(entity, agent, ws);
    }

    update_action(entity, agent, ws);
}*/
void Goap::process_agent(Entity entity, WorldState& ws, float /*dt*/) {
    auto& registry = engine.ecs.get_registry();
    auto& agent = registry.get<GoapAgent>(entity);

    // If the agent doesn't have a goal yet
    if (agent.needs_replan || agent.plan.empty()) {
        try_plan_goals(entity, agent, ws);
    }

    update_action(entity, agent, ws);
}

// ------------------------------------------------------
// Goal assignment
// ------------------------------------------------------
/**
 * Choose the highest priority relevant goal.
 *
 * Rules:
 *   - If agent already has an active, valid goal ? keep it.
 *   - Sort all available_goals by priority.
 *   - Choose the first goal whose conditions are not satisfied.
 *   - If all goals are satisfied ? pick lowest priority as fallback.
 */
void Goap::try_plan_goals(Entity entity, GoapAgent& agent, WorldState& ws) {
    if (agent.available_goals.empty()) {
        Log::warn("GOAP Agent {} has no goals.", entity);
        return;
    }

    // Sort goals by priority
    std::sort(agent.available_goals.begin(), agent.available_goals.end(), [](const GoapGoal& a, const GoapGoal& b) { return a.priority > b.priority; });

    GoapGoal* previous_goal = agent.has_goal() ? &agent.active_goal : nullptr;

    // Try goals in priority order
    for (auto& goal : agent.available_goals) {
        if (!goal.valid) continue;
        if (!goal.is_relevant(ws)) continue;
        if (ws.satisfies(goal.desired_state)) continue;

        // If this goal is already the active goal and plan exists, skip replanning
        if (previous_goal && previous_goal->name == goal.name && !agent.plan.empty()) {
            if (show_logging) {
                Log::info("GOAP Agent {} keeps current goal '{}'", entity, goal.name);
            }
            agent.active_goal = *previous_goal;
            agent.needs_replan = false;
            return;  // keep same goal & plan
        }

        // New goal, try to plan it
        agent.active_goal = goal;

        if (show_logging) {
            Log::info("GOAP Agent {} trying goal '{}'", entity, goal.name);
        }

        bool plan_success = update_plan(entity, agent, ws);

        if (plan_success) {
            agent.needs_replan = false;
            if (show_logging) {
                Log::info("GOAP Agent {} selected goal '{}'", entity, goal.name);
            }
            return;  // Found a goal that works
        } else {
            if (show_logging) {
                Log::warn("GOAP planning failed for goal '{}'", goal.name);
            }
            agent.active_goal.valid = false;  // mark as unachievable for now
        }
    }

    // No achievable goal found
    if (show_logging) {
        Log::warn("GOAP Agent {} could not satisfy any goals", entity);
    }
    agent.plan.clear();
    agent.current_action = nullptr;
    agent.active_goal.valid = false;
    agent.needs_replan = true;
}

// ------------------------------------------------------
// World state hashing for closed set
// ------------------------------------------------------
size_t hash_world_state(const WorldState& state) {
    size_t h = 0;
    for (const auto& [fact_id, value] : state.facts) {
        // Use FactId directly instead of hashing strings
        h ^= std::hash<uint32_t>()(fact_id) ^ std::hash<int>()(value << 1);
    }
    return h;
}

// ------------------------------------------------------
// Simple heuristic: number of unsatisfied goal facts
// ------------------------------------------------------
float heuristic_cost(const WorldState& state, const std::vector<FactPair>& goal) {
    float h = 0.f;
    for (const auto& fact : goal) {
        auto it = state.facts.find(fact.id.id);
        if (it == state.facts.end() || it->second != fact.value) {
            h += 1.f;
        }
    }
    return h;
}
// ------------------------------------------------------
// Helper: convert effective.preconditions (map) to vector<FactPair>
// ------------------------------------------------------
std::vector<FactPair> preconditions_to_vector(const std::unordered_map<std::string, bool>& preconds) {
    std::vector<FactPair> out;
    out.reserve(preconds.size());

    for (const auto& [key, value] : preconds) {
        FactPair f;
        f.id = FactId(key);  // construct from string
        f.value = value;
        out.push_back(f);
    }

    return out;
}

/**
 * Builds the runtime-effective version of a GOAP action.
 *
 * GOAP actions now have two layers of data:
 *   - The base action definition (registered in code)
 *   - The editor override data (loaded from saved JSON)
 *
 * This function merges them into a single EffectiveGoapAction:
 *   - The override is applied only when it exists and contains a valid field.
 *   - Base values act as the fallback default.
 *
 * This allows designers to modify values at runtime without affecting the
 * base registered action implementation.
 */
EffectiveGoapAction build_effective_action(const GoapAction& base, const GoapActionEditorData* override) {
    EffectiveGoapAction out;

    // --- Cost ---
    out.cost = (override && override->cost >= 0.f) ? override->cost : base.cost;

    // --- Preconditions ---
    out.preconditions = base.preconditions;
    if (override) {
        for (auto& [k, v] : override->preconditions) {
            out.preconditions[k] = v;
        }
    }

    // --- Effects ---
    out.effects = base.effects;
    if (override) {
        for (auto& [k, v] : override->effects) {
            out.effects[k] = v;
        }
    }

    return out;
}

/**
 * Builds a new plan to satisfy the current active goal.
 *
 * Process:
 *   - Interrupts any currently running action
 *   - Initialize open list with every action valid in current world state
 *   - Perform A* search
 *   - For each node: apply action effects and expand valid next actions
 *   - If goal satisfied -> reconstruct plan
 */
bool Goap::update_plan(Entity entity, GoapAgent& agent, WorldState& ws) {
    struct Node {
        GoapAction* action = nullptr;
        WorldState state;
        float cost_so_far = 0.f;
        float heuristic = 0.f;
        Node* parent = nullptr;
        float total_cost() const { return cost_so_far + heuristic; }
    };

    // Interrupt current action if any
    /*if (agent.current_action) {
        agent.current_action->on_interrupt(entity);
        agent.current_action->is_running = false;
        agent.current_action = nullptr;
    }*/
    if (agent.current_action) {
        // If we're replanning or the plan changed, interrupt
        if (agent.needs_replan || agent.plan[agent.current_index] != agent.current_action) {
            agent.current_action->on_interrupt(entity);
            agent.current_action->is_running = false;
            agent.current_action = nullptr;
        }
    }

    agent.clear_plan();

    // std::vector<Node> nodes;
    // nodes.reserve(128);
    std::deque<Node> nodes;

    using NodePtr = Node*;
    auto cmp = [](NodePtr a, NodePtr b) { return a->total_cost() > b->total_cost(); };
    std::priority_queue<NodePtr, std::vector<NodePtr>, decltype(cmp)> open_list(cmp);
    std::unordered_set<size_t> closed;

    // ------------------------------------------------------
    // Seed planner with all valid starting actions
    // ------------------------------------------------------
    for (GoapAction* action : agent.available_actions) {
        if (!action) continue;

        const auto* override = action_overrides.find(action->get_id());
        EffectiveGoapAction effective = build_effective_action(*action, override);

        auto preconds_vec = preconditions_to_vector(effective.preconditions);
        if (!ws.satisfies(preconds_vec)) continue;  // fixed hashing

        WorldState new_state = ws;
        action->apply_effects(new_state);

        nodes.push_back(Node { action, new_state, effective.cost, heuristic_cost(new_state, agent.active_goal.desired_state), nullptr });
        open_list.push(&nodes.back());
    }

    Node* goal_node = nullptr;

    // ------------------------------------------------------
    // A* search
    // ------------------------------------------------------
    while (!open_list.empty()) {
        Node* current = open_list.top();
        open_list.pop();
        if (!current) continue;

        size_t state_hash = hash_world_state(current->state);
        if (closed.count(state_hash)) continue;
        closed.insert(state_hash);

        if (current->state.satisfies(agent.active_goal.desired_state)) {
            goal_node = current;
            break;
        }

        for (GoapAction* next : agent.available_actions) {
            if (!next) continue;

            const auto* override = action_overrides.find(next->get_id());
            EffectiveGoapAction effective = build_effective_action(*next, override);

            auto preconds_vec = preconditions_to_vector(effective.preconditions);
            if (!current->state.satisfies(preconds_vec)) continue;

            WorldState next_state = current->state;
            next->apply_effects(next_state);

            float new_cost = current->cost_so_far + effective.cost;
            nodes.push_back(Node { next, next_state, new_cost, heuristic_cost(next_state, agent.active_goal.desired_state), current });
            open_list.push(&nodes.back());
        }
    }

    if (!goal_node) {
        Log::warn("GOAP planning failed for agent {}", entity);
        agent.active_goal.valid = false;
        return false;
    }

    std::vector<GoapAction*> plan;
    for (Node* n = goal_node; n; n = n->parent) {
        plan.push_back(n->action);
    }
    std::reverse(plan.begin(), plan.end());

    agent.plan = plan;
    agent.current_index = 0;
    agent.needs_replan = false;

    if (show_logging) {
        Log::info("GOAP plan for agent {} has {} steps", entity, plan.size());
        for (int i = 0; i < (int)plan.size(); i++) {
            Log::info("Step {}: {}", i, plan[i]->get_id());
        }
    }

    return true;
}

// ------------------------------------------------------
// Action execution
// ------------------------------------------------------
/**
 * Executes the current action in the plan.
 *
 * Logic:
 *   - If no current action -> start next one
 *   - If preconditions break -> interrupt and force replanning
 *   - Tick action logic
 *   - If finished:
 *       - apply effects
 *       - move to next action
 *       - if at end -> mark plan as complete
 */
void Goap::update_action(Entity entity, GoapAgent& agent, WorldState& ws) {
    if (agent.plan.empty()) {
        if (show_logging) Log::info("GOAP No plan for agent {}", entity);
        return;
    }

    GoapAction* action = agent.current_action;

    if (!action) {
        action = agent.plan[agent.current_index];
        agent.current_action = action;

        if (show_logging) Log::info("GOAP {} START (index = {})", action->get_id(), agent.current_index);

        action->is_running = true;
        action->on_start(entity);

        // Agent is now executing plan, no need to replan until necessary
        agent.needs_replan = false;
    }

    // Check if preconditions are still valid
    if (!action->check_preconditions(ws)) {
        if (show_logging) Log::info("GOAP {} INTERRUPT (preconditions failed)", action->get_id());

        action->on_interrupt(entity);
        action->is_running = false;
        agent.current_action = nullptr;
        agent.plan.clear();
        agent.needs_replan = true;
        return;
    }

    // Check if action is finished
    if (action->is_done(entity)) {
        if (show_logging) Log::info("GOAP {} FINISHED", action->get_id());

        action->apply_effects(ws);
        action->on_finished(entity);
        action->is_running = false;
        agent.current_action = nullptr;

        agent.current_index++;
        if (agent.current_index >= (int)agent.plan.size()) {
            if (show_logging) Log::info("GOAP Plan completed for agent {}", entity);
            agent.needs_replan = true;
        }
    }
}

}  // namespace tmt
