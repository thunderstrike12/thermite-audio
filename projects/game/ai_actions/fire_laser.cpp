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
    tmt::Log::error("laser onstart");
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    state = SITTING_DOWN;
    time = 0.0f;

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
    game::MediumEnemy& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    glm::vec3 enemy_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).get_world_position();
    glm::vec3 laser_pos = enemy_pos;
    if (tmt::engine.ecs.valid(enemy.laser_origin)) {
        tmt::Transform& laser_origin = tmt::engine.ecs.get_component<tmt::Transform>(enemy.laser_origin);
        laser_pos = laser_origin.get_world_position();
    }

    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    enemy.height_above_ground_offset = -enemy.height_above_ground + enemy.laser_sitting_down_height_offset;

    switch (state) {
        case SITTING_DOWN: {
            if (time > enemy.laser_sitting_down_time) {
                state = WINDING_UP;
                time = 0.0f;
            }
            break;
        }
        case WINDING_UP: {
            glm::vec3 target_dir = glm::normalize(target_pos - laser_pos);
            direction = target_dir;

            tmt::engine.polyline.use_color(0.5f, 0.1f, 0.1f);
            tmt::engine.polyline.use_line_width(8.0f);
            constexpr float START_DIST = 5.0f;
            float t = glm::clamp(time / enemy.laser_winding_up_time, 0.0f, 1.0f);
            dist_between_laser_spheres = glm::mix(START_DIST, 0.03f, t);

            for (float i = 0; i < 50.0f; i += dist_between_laser_spheres) {
                tmt::engine.polyline.draw_sphere(laser_pos + direction * i, .01f);
            }
            if (time > enemy.laser_winding_up_time) {
                state = FIRING;
                time = 0.0f;
            }
            break;
        }
        case FIRING: {
            auto linear_target_pos = target_pos;
            {
                glm::vec3 to_player = player_pos - linear_target_pos;
                if (glm::dot(to_player, to_player) > 0.0001f) {
                    glm::vec3 player_vel = (player_pos - last_player_pos) / dt;
                    last_player_pos = player_pos;
                    linear_target_pos = linear_target_pos + glm::normalize(to_player) * enemy.laser_linear_speed * dt + player_vel * enemy.laser_prediction_length;
                }
            }

            auto exponential_target_pos = target_pos;
            {
                glm::vec3 to_player = player_pos - exponential_target_pos;
                float length = glm::length(to_player);
                if (glm::dot(to_player, to_player) > 0.0001f) {
                    glm::vec3 player_vel = (player_pos - last_player_pos) / dt;
                    last_player_pos = player_pos;
                    exponential_target_pos = exponential_target_pos + glm::normalize(to_player) * enemy.laser_exponential_speed * length * dt + player_vel * enemy.laser_prediction_length;
                }
            }

            float linear_weight = glm::clamp(time / enemy.laser_linear_threshold, 0.0f, 1.0f);
            float exponential_weight = 1.0f - linear_weight;
            target_pos = linear_target_pos * linear_weight + exponential_target_pos * exponential_weight;

            glm::vec3 target_dir = glm::normalize(target_pos - laser_pos);
            direction = target_dir;

            if (time > enemy.laser_firing_time) {
                auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
                enemy.laser_timer = 0.0f;
                enemy.height_above_ground_offset = 0.0f;
                auto ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(enemy_entity);
                if (!ws) return;
                ws->set_fact(tmt::FactId("m_laser_ready"), false);
            }

            tmt::engine.polyline.use_color(1.0f, 0.1f, 0.1f);
            tmt::engine.polyline.use_line_width(10.0f);
            for (float i = 0; i < 50.0f; i += 0.03f) {
                tmt::engine.polyline.draw_sphere(laser_pos + direction * i, .01f);
            }

            break;
        }
        default: {
        }
    }
    time += dt;
}

bool FireLaser::is_done(tmt::Entity /*enemy_entity*/) const {
    return false;
}