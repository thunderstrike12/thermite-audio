#include "fire_laser.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <cstdlib>

void FireLaser::on_start(tmt::Entity enemy_entity) {
    for (const auto& [cam_entity, camera] : tmt::engine.ecs.view<tmt::Camera>().each()) {
        player = cam_entity;
        break;
    }
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    duration = enemy.laser_duration;

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    direction = player_pos - enemy_entity_pos +
                glm::vec3(
                    (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness,
                    (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness
                );
    direction = glm::normalize(direction);
    target_pos = player_pos + glm::vec3(
                                  (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness,
                                  (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness
                              );
}

void FireLaser::on_tick(tmt::Entity enemy_entity, float dt) {
    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    game::MediumEnemy enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    switch (enemy.laser_mode) {
        case game::MediumEnemy::LINEAR: {
            glm::vec3 target_dir = glm::normalize(player_pos - enemy_entity_pos);
            glm::vec3 axis = glm::cross(direction, target_dir);
            if (glm::length(axis) > 0.001f) {
                axis = glm::normalize(axis);
                float angle = glm::acos(glm::clamp(glm::dot(direction, target_dir), -1.0f, 1.0f));
                float rotate_by = glm::min(angle, enemy.laser_accuracy * dt);
                direction = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0f), rotate_by, axis) * glm::vec4(direction, 0.0f)));
            }
            break;
        }
        case game::MediumEnemy::EXPONENTIAL: {
            glm::vec3 to_player = player_pos - target_pos;
            float length = glm::length(to_player);
            if (glm::dot(to_player, to_player) > 0.0001f) {
                glm::vec3 player_vel = (player_pos - last_player_pos) / dt;
                last_player_pos = player_pos;
                target_pos = target_pos + glm::normalize(to_player) * enemy.laser_accuracy * length + player_vel * enemy.laser_prediction_length;
            }
            glm::vec3 target_dir = glm::normalize(target_pos - enemy_entity_pos);
            direction = target_dir;
            break;
        }
        default: {
        }
    }

    if (duration < 0.0f) {
        auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
        enemy.laser_timer = 0.0f;
        auto ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(enemy_entity);
        if (!ws) return;
        // ws->facts[std::hash<std::string>()("m_laser_ready")] = false;
        ws->set_fact(tmt::FactId("m_laser_ready"), false);
    }

    tmt::engine.polyline.use_line_width(10.0f);
    // tmt::engine.polyline.draw_line(enemy_entity_pos, enemy_entity_pos + direction * 50.0f);
    // draw spheres along the line for better visibility
    for (float i = 0; i < 50.0f; i += 0.1f) {
        tmt::engine.polyline.draw_sphere(enemy_entity_pos + direction * i, .01f);
    }
    duration -= dt;
}

bool FireLaser::is_done(tmt::Entity enemy_entity) const {
    return false;
}
