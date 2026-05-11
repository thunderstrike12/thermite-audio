#include "fire_laser.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "../components/gameplay_functionality_components/player.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <cstdlib>

#include "engine/core/renderer/renderer.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"

void FireLaser::cleanup(tmt::Entity enemy_entity) {
    game::MediumEnemy& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    if (tmt::engine.ecs.valid(laser_entity)) {
        tmt::engine.ecs.destroy_entity(laser_entity);
    }
    laser_entity = entt::null;
    state = WINDING_UP;
    enemy.laser_timer = 0.0f;
    enemy.height_above_ground_offset = 0.0f;
    auto* ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(enemy_entity);
    if (!ws) return;
    ws->set_fact(tmt::FactId("m_laser_ready"), false);
}

void FireLaser::on_start(tmt::Entity enemy_entity) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    state = SITTING_DOWN;
    time = 0.0f;

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy.player).get_world_position();

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

    auto player_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy.player).get_world_position();
    player_pos += enemy.laser_target_offset;

    switch (state) {
        case SITTING_DOWN: {
            enemy.height_above_ground_offset = -enemy.height_above_ground + enemy.laser_sitting_down_height_offset;
            if (time > enemy.laser_sitting_down_time) {
                state = WINDING_UP;
                time = 0.0f;
            }
            break;
        }
        case WINDING_UP: {
            enemy.height_above_ground_offset = -enemy.height_above_ground + enemy.laser_sitting_down_height_offset;
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
                laser_entity = tmt::engine.ecs.create_entity();
                auto& voxel_renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(laser_entity);
                auto ref = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(enemy.laser_voxel_object);
                voxel_renderer.resource = ref;
                // auto& voxel_body = tmt::engine.ecs.add_component<tmt::VoxelBody>(laser_entity);
                // voxel_body.layer = enemy.projectile_layer;
                // voxel_body.type = tmt::VoxelBody::STATIC;

                state = FIRING;
                time = 0.0f;
            }
            break;
        }
        case FIRING: {
            if (!tmt::engine.ecs.valid(laser_entity)) {
                tmt::Log::error("laser entity not available, but fire laser action called");
                laser_entity = entt::null;
                state = WINDING_UP;
                enemy.laser_timer = 0.0f;
                enemy.height_above_ground_offset = 0.0f;
                auto* ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(enemy_entity);
                if (!ws) return;
                ws->set_fact(tmt::FactId("m_laser_ready"), false);

                return;
            }

            glm::vec3 raw_vel = (player_pos - last_player_pos) / dt;
            last_player_pos = player_pos;

            smoothed_player_vel = glm::mix(smoothed_player_vel, raw_vel, enemy.laser_vel_smoothing);
            glm::vec3 predicted_player_pos = player_pos + smoothed_player_vel * (enemy.laser_prediction_length / (1.0f / glm::distance(player_pos, target_pos)));

            auto linear_target_pos = target_pos;
            {
                glm::vec3 to_player = predicted_player_pos - linear_target_pos;
                float dist = glm::length(to_player);
                if (dist > 0.0001f) {
                    float step = enemy.laser_linear_speed * dt;
                    linear_target_pos += (to_player / dist) * glm::min(step, dist);
                }
            }

            auto exponential_target_pos = target_pos;
            {
                glm::vec3 to_player = predicted_player_pos - exponential_target_pos;
                float dist = glm::length(to_player);
                if (dist > 0.0001f) {
                    float step = enemy.laser_exponential_speed * dist * dt;
                    exponential_target_pos += (to_player / dist) * glm::min(step, dist);
                }
            }

            float linear_weight = glm::clamp(time / enemy.laser_linear_threshold, 0.0f, 1.0f);
            float exponential_weight = 1.0f - linear_weight;
            target_pos = linear_target_pos * linear_weight + exponential_target_pos * exponential_weight;
            direction = glm::normalize(target_pos - laser_pos);

            glm::vec3 target_dir = glm::normalize(target_pos - laser_pos);
            direction = target_dir;

            if (time > enemy.laser_firing_time) {
                cleanup(enemy_entity);
                return;
            }

            // scaling laser and checking for player collision
            const tmt::Ray ray_cast = tmt::Ray(laser_pos, glm::normalize(direction));
            const tmt::Hit laser_hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray_cast, enemy.projectile_mask);
            auto hit_pos = laser_pos + direction * laser_hit.distance;

            const float MAX_RANGE = 50.0f;
            float raycast_dist = (laser_hit.entity != entt::null) ? laser_hit.distance : MAX_RANGE;
            raycast_dist = glm::min(raycast_dist, MAX_RANGE);
            glm::vec3 end_pos = laser_pos + direction * raycast_dist;

            // closest point on laser to player
            glm::vec3 line = end_pos - laser_pos;
            float line_len_sq = glm::dot(line, line);
            float t = glm::clamp(glm::dot(player_pos - laser_pos, line) / line_len_sq, 0.0f, 1.0f);
            glm::vec3 closest_point = laser_pos + line * t;
            float distance_to_player = glm::distance(closest_point, player_pos);

            // deal damage to player and cutoff laser
            if (distance_to_player < enemy.laser_damage_radius) {
                tmt::engine.ecs.get_component<game::Player>(enemy.player).health.value -= enemy.laser_damage * dt;
                // end_pos = closest_point;
            }

            // scale laser voxel object between laser hit and laser pos
            auto& laser_transform = tmt::engine.ecs.get_component<tmt::Transform>(laser_entity);
            float laser_length = glm::distance(laser_pos, end_pos);
            glm::vec3 mid_point = (laser_pos + end_pos) * 0.5f;
            laser_transform.set_world_position(mid_point);

            glm::vec3 up = (glm::abs(glm::dot(direction, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
            laser_transform.set_world_rotation(glm::quatLookAt(direction, up));
            laser_transform.set_world_scale(glm::vec3(0.1f, 0.1f, laser_length));

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

void FireLaser::on_interrupt(tmt::Entity enemy_entity) {
    cleanup(enemy_entity);
}