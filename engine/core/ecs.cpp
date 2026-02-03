#include "ecs.hpp"

namespace tmt {

void Ecs::on_game_start() {
    for (auto& system : systems) {
        system->on_start();
    }
}

void Ecs::on_game_update(const FrameData& time) {
    for (auto& system : systems) {
        system->on_update(time);
    }
}

void Ecs::on_game_fixed_update(const FrameData& time) {
    for (auto& system : systems) {
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
        auto& transform = get_component<Transform>(entity);
        // If the entity's parent is valid (and wasn't destroyed before this entity) we then also clear its parent to not leave relationships between invalid entities.
        if (valid(transform.get_parent())) transform.clear_parent();

        registry.destroy(entity);
    }
}

}  // namespace tmt
