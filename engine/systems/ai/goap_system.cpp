#include "goap_system.hpp"
#include "core/logger.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"

#include <queue>
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace tmt {

void Goap::on_start() { Log::info("Goap on_start"); }

void Goap::on_update(const FrameData& time) {
    auto& registry = engine.ecs.get_registry();
    float dt = time.delta_time;

    for (auto [entity, agent, world] : registry.view<GoapAgent, WorldState>().each()) {
        process_agent(entity, world, dt);
    }
}

void Goap::on_fixed_update(const FrameData&) {}

void Goap::on_end() { Log::info("Goap on_end"); }

// ------------------------------------------------------
// Main per-agent update
// ------------------------------------------------------
void Goap::process_agent(Entity entity, WorldState& ws, float dt) {
    auto& registry = engine.ecs.get_registry();
    auto& agent = registry.get<GoapAgent>(entity);

    update_goal(entity, agent, ws);
    if (agent.needs_replan) update_plan(entity, agent, ws);
    update_action(entity, agent, dt);
}

// ------------------------------------------------------
// Step 1: goal assignment
// ------------------------------------------------------
void Goap::update_goal(Entity entity, GoapAgent& agent, WorldState& ws) {
    // If agent already has an active goal, do nothing
    if (agent.has_goal()) return;

    if (agent.available_goals.empty()) {
        Log::warn("GOAP Agent {} has no goals.", entity);
        return;
    }

    // Sort goals by descending priority
    std::sort(agent.available_goals.begin(), agent.available_goals.end(), [](const GoapGoal& a, const GoapGoal& b) { return a.priority > b.priority; });

    // Select first goal that is relevant
    for (const auto& goal : agent.available_goals) {
        if (goal.is_relevant(ws)) {
            agent.active_goal = goal;
            agent.needs_replan = true;

            Log::info("GOAP Agent {} selected goal '{}' with priority {}", entity, agent.active_goal.name, goal.priority);
            return;
        }
    }

    // If all goals satisfied, pick lowest priority (fallback)
    agent.active_goal = agent.available_goals.back();
    agent.needs_replan = true;

    Log::info("GOAP Agent {} selected fallback goal.", entity);
}

// ------------------------------------------------------
// A* GOAP Planner
// ------------------------------------------------------
struct Node {
    GoapAction* action;
    float cost_so_far;
    float heuristic;
    Node* parent;

    Node(GoapAction* a, float cost, float h, Node* p) : action(a), cost_so_far(cost), heuristic(h), parent(p) {}

    float total_cost() const { return cost_so_far + heuristic; }
};

// ------------------------------------------------------
// Step 2: planning
// ------------------------------------------------------
void Goap::update_plan(Entity entity, GoapAgent& agent, WorldState& ws) {
    agent.clear_plan();

    // Storage for nodes
    std::vector<Node> nodes;
    nodes.reserve(64);  // avoid vector reallocation -> prevents pointer invalidation

    using NodePtr = Node*;

    // Min-heap priority queue by total_cost
    auto cmp = [](NodePtr a, NodePtr b) { return a->total_cost() > b->total_cost(); };
    std::priority_queue<NodePtr, std::vector<NodePtr>, decltype(cmp)> open_list(cmp);

    // Closed set: storing only raw pointers to GoapAction
    std::unordered_set<GoapAction*> closed;
    closed.reserve(agent.actions.size());

    // ------------------------------------------------------
    // Log all available actions
    // ------------------------------------------------------
    /*Log::info("GOAP: Agent {} has {} actions:", int(entity), int(agent.actions.size()));
    for (auto& action_ptr : agent.actions) {
        if (!action_ptr) continue;
        Log::info("  - {}", action_ptr->get_name());
    }*/

    // ------------------------------------------------------
    // Add all valid starting actions (whose preconditions match current worldstate)
    // ------------------------------------------------------
    for (auto& action_ptr : agent.actions) {
        GoapAction* action = action_ptr.get();
        if (!action) continue;

        if (action->check_preconditions(ws)) {
            nodes.emplace_back(action, action->cost, 0.f, nullptr);
            Node* node = &nodes.back();
            open_list.push(node);
            // Log::info("GOAP: Starting action '{}' satisfies preconditions, added to open list", action->get_name());
        } else {
            // Log::info("GOAP: Action '{}' does NOT satisfy preconditions", action->get_name());
        }
    }

    Node* goal_node = nullptr;
    int iteration = 0;

    // ------------------------------------------------------
    // A* SEARCH LOOP
    // ------------------------------------------------------
    while (!open_list.empty()) {
        Node* current = open_list.top();
        open_list.pop();
        iteration++;

        if (!current) continue;

        // If this action already processed, skip
        if (closed.count(current->action)) continue;
        closed.insert(current->action);

        // simulate effects
        WorldState temp = ws;
        current->action->apply_effects(temp);

        // check if goal met
        if (temp.satisfies(agent.active_goal.desired_state)) {
            goal_node = current;
            // Log::info("GOAP: Goal satisfied by action '{}'", current->action->get_name());
            break;
        }

        // Expand children: all actions whose preconditions match temp state
        for (auto& action_ptr : agent.actions) {
            GoapAction* next = action_ptr.get();
            if (!next) continue;

            // skip if closed
            if (closed.count(next)) continue;

            if (next->check_preconditions(temp)) {
                float new_cost = current->cost_so_far + next->cost;

                nodes.emplace_back(next, new_cost, 0.f, current);
                Node* new_node = &nodes.back();
                open_list.push(new_node);

                // Log::info("GOAP: Adding child action '{}' to open list (cost: {})", next->get_name(), new_cost);
            } else {
                // Log::info("GOAP: Child action '{}' preconditions NOT satisfied at this node", next->get_name());
            }
        }
    }

    // ------------------------------------------------------
    // Reconstruct plan
    // ------------------------------------------------------
    if (!goal_node) {
        Log::warn("GOAP: planning FAILED for agent {} after {} iterations", entity, iteration);
        return;
    }

    std::vector<GoapAction*> plan;

    for (Node* n = goal_node; n; n = n->parent) plan.push_back(n->action);

    std::reverse(plan.begin(), plan.end());

    agent.plan = plan;
    agent.current_index = 0;
    agent.needs_replan = false;

    Log::info("GOAP: Plan created for agent {} with {} steps", entity, int(plan.size()));
    for (int i = 0; i < (int)plan.size(); ++i) {
        Log::info("  Step {}: {}", i, plan[i]->get_name());
    }
}

// ------------------------------------------------------
// Step 3: action execution
// ------------------------------------------------------
void Goap::update_action(Entity entity, GoapAgent& agent, float dt) {
    GoapAction* action = agent.get_current_action();
    if (!action) return;  // no plan yet

    // TODO: logic for running the action
}

}  // namespace tmt
