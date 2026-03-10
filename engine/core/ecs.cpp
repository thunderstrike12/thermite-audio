#include "ecs.hpp"
#include "engine/events/ecs.hpp"

namespace tmt {

bool Ecs::is_enabled(const Entity entity) const {
    return !has_component<Disable>(entity);
}

bool Ecs::is_disabled(const Entity entity) const {
    return has_component<Disable>(entity);
}

void Ecs::enable(const Entity entity, const bool mark) {
    if (mark) remove_component<DisableFlag>(entity);  // clear explicit intent

    /* Only become effectively enabled if parent isn't disabled */
    const Entity parent = get_component<Transform>(entity).get_parent();
    const bool parent_is_disabled = EntityHelper::is_valid(parent) && has_component<Disable>(parent);

    if (!parent_is_disabled) {
        remove_component<Disable>(entity);
        OnEnableEntity::dispatch(entity);
        propagate_enable(entity);
    }
}

void Ecs::propagate_enable(const Entity entity) {
    for (const auto child : get_component<Transform>(entity).get_children()) {
        if (has_component<DisableFlag>(child)) {
            /* this child is explicitly disabled, stop this branch entirely */
            continue;
        }
        remove_component<Disable>(child);
        OnEnableEntity::dispatch(child);
        propagate_enable(child);  // recurse into children
    }
}

void Ecs::disable(const Entity entity, const bool mark) {
    if (mark) add_or_get_component<DisableFlag>(entity);
    add_or_get_component<Disable>(entity);
    OnDisableEntity::dispatch(entity);

    const auto children = get_component<Transform>(entity).get_all_children();
    for (const auto child : children) {
        add_or_get_component<Disable>(child);
        OnDisableEntity::dispatch(child);
    }
}

void Ecs::on_game_start() {
    for (auto& system : systems) {
        system->on_start();
    }
}

void Ecs::on_game_update(const FrameData& time) {
    for (auto& system : systems) {
        if (system->is_disabled()) continue;
        system->on_update(time);
    }
}

void Ecs::on_game_fixed_update(const FrameData& time) {
    for (auto& system : systems) {
        if (system->is_disabled()) continue;
        system->on_fixed_update(time);
    }
}

void Ecs::on_game_end() {
    for (auto& system : systems) {
        system->on_end();
    }
}

void Ecs::on_end_frame() {
    auto view = registry.view<Delete>();

    for (auto entity : view) {
        /* Need to use try get since we can techincally destroy empty entities */
        auto* transform = try_get_component<Transform>(entity);
        // If the entity's parent is valid (and wasn't destroyed before this entity) we then also clear its parent to not leave relationships between invalid entities.
        if (transform && valid(transform->get_parent())) transform->clear_parent();

        registry.destroy(entity);
    }
}

}  // namespace tmt
