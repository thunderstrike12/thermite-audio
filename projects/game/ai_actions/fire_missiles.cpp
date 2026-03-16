#include "fire_missiles.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <cstdlib>

void FireMissiles::on_start(tmt::Entity enemy_entity) {
    for (const auto& [CamEntity, camera] : tmt::engine.ecs.view<tmt::Camera>().each()) {
        player = CamEntity;
        break;
    }
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    missiles = enemy.missile_burst;

}

void FireMissiles::on_tick(tmt::Entity enemy_entity, float dt) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(enemy.walkable_asteroid);

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    // Follow path to player
    std::optional<glm::vec3> direction = nav_mesh.follow_path(enemy_entity_pos, player_pos);

    // If path has more waypoints, move to next one
    if (direction) {
        auto velocity = glm::vec3(*direction * enemy.walk_speed);
        enemy.velocity += velocity;
    } else {
        // Close enough to consider node reached, force path recompute
        nav_mesh.path.clear();
    }

    interval_timer += dt;
    if (interval_timer > enemy.burst_interval) {
        interval_timer -= enemy.burst_interval;
        Missile missile;
        missile = enemy;
        missile.position = enemy_entity_pos;

        //set random offset
        missile.offset = glm::vec3(
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness,
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness
        );

        enemy.missiles.emplace_back(missile);
        missiles--;
        
    }
    
    if (missiles == 0) {
        auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
        enemy.missile_timer = interval_timer * enemy.missile_burst;
        auto ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(enemy_entity);
        if (!ws) return;
        ws->facts[std::hash<std::string>()("m_missiles_ready")] = false;
    }
}

bool FireMissiles::is_done(tmt::Entity enemy_entity) const {
    return false;
}
