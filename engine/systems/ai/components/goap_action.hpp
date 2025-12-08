#pragma once
#include "engine/core/ecs.hpp"
#include "world_state.hpp"

namespace tmt {

class GoapAction {
   public:
    GoapAction() = default;
    virtual ~GoapAction() = default;

    // These are used by the planner
    std::unordered_map<std::string, bool> preconditions;
    std::unordered_map<std::string, bool> effects;
    float cost = 1.f;

    // readable name
    virtual const char* get_name() const = 0;

    // Planner helper functions
    virtual bool check_preconditions(const WorldState& ws) const;
    virtual void apply_effects(WorldState& ws) const;  // can call apply() from ws

    // action execuion helpers
    virtual void on_start(entt::entity agent, entt::registry& ecs) {}
    virtual void on_update(entt::entity agent, entt::registry& ecs, float dt) {}
    // virtual bool is_finished(entt::entity agent, entt::registry& ecs) const { return true; }
};

}  // namespace tmt
