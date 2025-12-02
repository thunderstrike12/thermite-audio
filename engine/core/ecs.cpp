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
        registry.destroy(entity);
    }
}

}  // namespace tmt
