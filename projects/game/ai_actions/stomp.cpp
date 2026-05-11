#include "stomp.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
// todo: make projects not have to use realtive paths
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "../components/gameplay_functionality_components/player.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/polyline.hpp"

#include <cstdlib>

void Stomp::on_start(tmt::Entity enemy_entity) {
    time = 0.0f;
}

void Stomp::on_tick(tmt::Entity enemy_entity, float dt) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    time += dt;
    if (time > enemy.stomp_windup) {
        enemy.stomp_timer = 0.0f;
        tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
        const auto& enemy_entity_pos = enemy_transform.get_world_position();

        // if player in range, deal damage
        auto& player_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy.player);
        const auto& player_pos = player_transform.get_world_position();
        if (glm::length(player_pos - enemy_entity_pos) <= enemy.stomp_radius) {
            tmt::engine.ecs.get_component<game::Player>(enemy.player).health.value -= enemy.stomp_damage;
        }

        tmt::engine.polyline.use_color(0, 1, 0);
        tmt::engine.polyline.use_line_width(20.0f);
        tmt::engine.polyline.draw_sphere(enemy_entity_pos, enemy.stomp_radius, 128, 0.5f);
    }
}

bool Stomp::is_done(tmt::Entity enemy_entity) const {
    return false;
}