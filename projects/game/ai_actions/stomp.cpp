#include "stomp.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
// todo: make projects not have to use realtive paths
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/polyline.hpp"

#include <cstdlib>

void Stomp::on_start(tmt::Entity enemy_entity) {
    for (const auto& [cam_entity, camera] : tmt::engine.ecs.view<tmt::Camera>().each()) {
        player = cam_entity;
        break;
    }
    time = 0.0f;
}

void Stomp::on_tick(tmt::Entity enemy_entity, float dt) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    time += dt;
    if (time > enemy.stomp_windup) {
        enemy.stomp_timer = 0.0f;
        tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
        const auto& enemy_entity_pos = enemy_transform.get_world_position();
        tmt::engine.polyline.use_color(0, 1, 0);
        tmt::engine.polyline.use_line_width(20.0f);
        tmt::engine.polyline.draw_sphere(enemy_entity_pos, enemy.stomp_radius, 128, 0.5f);
    }
}

bool Stomp::is_done(tmt::Entity enemy_entity) const {
    return false;
}