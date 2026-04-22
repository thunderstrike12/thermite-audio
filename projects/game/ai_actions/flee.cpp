#include "flee.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <cstdlib>

void Flee::on_start(tmt::Entity enemy_entity) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    enemy.back_off_distance = 99999999.9f;
}

void Flee::on_tick(tmt::Entity enemy_entity, float dt) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    enemy.kite_player();
}

bool Flee::is_done(tmt::Entity) const {
    return false;
}
