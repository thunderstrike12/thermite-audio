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
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

void FireLaser::cleanup(tmt::Entity enemy_entity) {
    game::MediumEnemy& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    // Reset voxel entities to their rest pose so the rig can resume cleanly.
    auto& voxels = enemy.laser_voxel_entities;
    if (!voxels.empty() && rest_local_positions.size() == voxels.size()) {
        auto& et = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
        glm::vec3 ep = et.get_world_position();
        glm::quat er = et.get_world_rotation();
        for (size_t i = 0; i < voxels.size(); ++i) {
            auto voxel = voxels[i];
            if (!tmt::engine.ecs.valid(voxel)) continue;
            auto& vt = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
            vt.set_world_position(ep + er * rest_local_positions[i]);
            vt.set_world_rotation(er * rest_local_rotations[i]);
        }
    }
    enemy.set_stored_offsets_to_base();
    rest_local_positions.clear();
    rest_local_rotations.clear();
    tmt::engine.ecs.get_component<tmt::RigController>(enemy.rig_controller).set_parameter_bool("laser", false);
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

void FireLaser::rotate_to_face_player(tmt::Entity enemy_entity, float dt) {
    game::MediumEnemy& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    glm::vec3 enemy_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).get_world_position();
    auto player_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy.player).get_world_position();

    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(enemy.walkable_asteroid);
    if (!nav_mesh.nav_nodes_valid()) return;

    int closest_node = nav_mesh.find_closest_node(enemy_pos);
    auto dir = nav_mesh.follow_path(enemy_pos, player_pos);
    if (!dir) return;
    auto* nodes = nav_mesh.nodes_mesh;
    auto& normal = (*nodes)[closest_node].normal;
    glm::vec3 ground_up = glm::normalize(normal);
    glm::quat rot_velocity = glm::quat(1, 0, 0, 0);
    if (glm::dot(*dir, *dir) > 0.00001f) {
        // project velocity onto tangent plane of the ground
        glm::vec3 forward = *dir - ground_up * glm::dot(*dir, ground_up);

        if (glm::dot(forward, forward) > 0.00001f) {
            forward = glm::normalize(forward);
            glm::vec3 right = glm::normalize(glm::cross(ground_up, forward));
            glm::vec3 up = glm::cross(forward, right);

            glm::mat3 basis(right, up, forward);
            rot_velocity = glm::normalize(glm::quat_cast(basis));
        }
    }
    float f = dt * enemy.rotation_speed_during_laser;
    enemy.rotation = glm::slerp(enemy.rotation, rot_velocity, glm::clamp(f, 0.0f, 1.0f));
    enemy.rotation = glm::normalize(enemy.rotation);
    tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).set_world_rotation(enemy.rotation);
}

void FireLaser::update_formation(tmt::Entity enemy_entity, float aim_blend) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    auto& voxels = enemy.laser_voxel_entities;
    if (voxels.empty() || rest_local_positions.size() != voxels.size()) return;

    auto& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    glm::vec3 enemy_pos = enemy_transform.get_world_position();
    glm::quat enemy_rot = enemy_transform.get_world_rotation();

    auto rest_world_pos = [&](size_t i) { return enemy_pos + enemy_rot * rest_local_positions[i]; };
    auto rest_world_rot = [&](size_t i) { return enemy_rot * rest_local_rotations[i]; };

    glm::vec3 pivot_pos = rest_world_pos(voxels.size() - 1);
    glm::vec3 rest_forward_world = glm::normalize(enemy_rot * rest_local_forward);

    glm::vec3 dir = glm::normalize(direction);
    glm::quat aim_rotation;
    float cos_a = glm::dot(rest_forward_world, dir);
    if (cos_a > 0.99999f) {
        aim_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    } else if (cos_a < -0.99999f) {
        glm::vec3 axis = glm::cross(rest_forward_world, glm::vec3(0.0f, 1.0f, 0.0f));
        if (glm::dot(axis, axis) < 0.0001f) axis = glm::cross(rest_forward_world, glm::vec3(1.0f, 0.0f, 0.0f));
        aim_rotation = glm::angleAxis(glm::pi<float>(), glm::normalize(axis));
    } else {
        aim_rotation = glm::rotation(rest_forward_world, dir);
    }

    aim_blend = glm::clamp(aim_blend, 0.0f, 1.0f);
    aim_rotation = glm::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), aim_rotation, aim_blend);

    for (size_t i = 0; i < voxels.size(); ++i) {
        auto voxel = voxels[i];
        if (!tmt::engine.ecs.valid(voxel)) continue;
        auto& vt = tmt::engine.ecs.get_component<tmt::Transform>(voxel);

        glm::vec3 rp = rest_world_pos(i);
        glm::quat rr = rest_world_rot(i);

        vt.set_world_position(pivot_pos + aim_rotation * (rp - pivot_pos));
        vt.set_world_rotation(aim_rotation * rr);
    }
}

void FireLaser::on_start(tmt::Entity enemy_entity) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    Tweening::tween<float>()  //
        .from(1.0f)
        .to(0.0f)
        .duration(0.2f)
        .ease(Tweening::Ease::IN_OUT_QUAD)
        .on_update([enemy_entity](float alpha, const float* value) {
            //
            auto& enemy_comp = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
            tmt::engine.ecs.get_component<tmt::ConstrainedRig>(enemy_comp.rig_controller).blend = *value;
        });
    state = SITTING_DOWN;
    time = 0.0f;

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy.player).get_world_position();

    // target_pos = player_pos + glm::vec3(
    //                               (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness,
    //                               (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness
    //                           );
    target_pos = enemy_transform.get_world_position() + enemy_transform.get_forward() * enemy.laser_range * 0.8f;
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
            if (enemy.audio_emitter != nullptr && !enemy.shield_slam_instance.is_valid()) {
                enemy.shield_slam_instance = enemy.audio_emitter->play(enemy.sounds.sound_shield_slam);
                enemy.shield_slam_instance.set_maximum_distance(enemy.aggro_range * 1.25f);  // Multiply be 1.25f to ensure the player can hear it even when at the edge of the range
            }

            tmt::engine.ecs.get_component<tmt::RigController>(enemy.rig_controller).set_parameter_bool("laser", true);
            enemy.height_above_ground_offset = -enemy.height_above_ground + enemy.laser_sitting_down_height_offset;
            if (time > enemy.laser_sitting_down_time) {
                // auto& constrained_rig = tmt::engine.ecs.get_component<tmt::ConstrainedRig>(enemy.rig_controller);
                // constrained_rig.copy_reference_pose_from_keyframe(tmt::engine.ecs.get_component<tmt::RigModel>(enemy.rig_controller));
                Tweening::tween<float>()  //
                    .from(0.0f)
                    .to(1.0f)
                    .duration(0.2f)
                    .ease(Tweening::Ease::IN_OUT_QUAD)
                    .on_update([enemy_entity](float alpha, const float* value) {
                        //
                        auto& enemy_comp = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
                        tmt::engine.ecs.get_component<tmt::ConstrainedRig>(enemy_comp.rig_controller).blend = *value;
                    });
                enemy.set_stored_offsets_to_ref_entity();
                state = WINDING_UP;
                time = 0.0f;
            }
            break;
        }
        case WINDING_UP: {
            rotate_to_face_player(enemy_entity, dt);
            tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
            target_pos = enemy_transform.get_world_position() + enemy_transform.get_forward() * enemy.laser_range * 0.8f;

            if (enemy.audio_emitter != nullptr && !enemy.laser_instance.is_valid()) {
                enemy.laser_instance = enemy.audio_emitter->play(enemy.sounds.sound_laser);
                enemy.laser_instance.set_maximum_distance(enemy.aggro_range * 1.25f);  // Multiply be 1.25f to ensure the player can hear it even when at the edge of the range
            }

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
            // move the laser voxels into position with a tween
            const float tween_start_time = enemy.laser_winding_up_time - enemy.laser_aim_tween_duration;
            if (enemy.laser_aim_tween_duration > 0.0f && time >= tween_start_time) {
                if (rest_local_positions.size() != enemy.laser_voxel_entities.size()) {
                    auto& et = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
                    glm::vec3 cache_enemy_pos = et.get_world_position();
                    glm::quat enemy_rot_inv = glm::inverse(et.get_world_rotation());

                    rest_local_positions.clear();
                    rest_local_rotations.clear();
                    rest_local_positions.reserve(enemy.laser_voxel_entities.size());
                    rest_local_rotations.reserve(enemy.laser_voxel_entities.size());

                    for (auto voxel : enemy.laser_voxel_entities) {
                        if (!tmt::engine.ecs.valid(voxel)) {
                            rest_local_positions.push_back(glm::vec3(0.0f));
                            rest_local_rotations.push_back(glm::quat(1, 0, 0, 0));
                            continue;
                        }
                        auto& vt = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
                        rest_local_positions.push_back(enemy_rot_inv * (vt.get_world_position() - cache_enemy_pos));
                        rest_local_rotations.push_back(enemy_rot_inv * vt.get_world_rotation());
                    }
                    if (rest_local_positions.size() >= 2) {
                        glm::vec3 tip_to_back_local = rest_local_positions.front() - rest_local_positions.back();
                        if (glm::dot(tip_to_back_local, tip_to_back_local) > 1e-8f) {
                            rest_local_forward = glm::normalize(tip_to_back_local);
                        } else {
                            rest_local_forward = glm::vec3(0.0f, 0.0f, 1.0f);
                        }
                    } else {
                        rest_local_forward = glm::vec3(0.0f, 0.0f, 1.0f);
                    }
                }

                float blend = glm::clamp((time - tween_start_time) / enemy.laser_aim_tween_duration, 0.0f, 1.0f);
                float eased = blend * blend * (3.0f - 2.0f * blend);  // smoothstep
                update_formation(enemy_entity, eased);
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

            rotate_to_face_player(enemy_entity, dt);
            update_formation(enemy_entity, 1.0f);

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
            const tmt::Hit laser_hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray_cast, enemy.enemy_mask);
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
                // tmt::engine.ecs.get_component<game::Player>(enemy.player).take_damage(enemy.laser_damage * dt);
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
