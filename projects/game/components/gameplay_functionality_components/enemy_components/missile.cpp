#include "missile.hpp"
#include "missile.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"

void game::Missile::start() {
    auto& name = tmt::engine.ecs.get_component<tmt::Name>(entity);
    name.name = "Missile";

    MediumEnemy& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    glm::vec3 enemy_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).get_world_position();
    glm::vec3 missile_pos = enemy_pos;
    if (tmt::engine.ecs.valid(enemy.missile_origin)) {
        tmt::Transform& missile_origin = tmt::engine.ecs.get_component<tmt::Transform>(enemy.missile_origin);
        missile_pos = missile_origin.get_world_position();
    }
    auto& missile_trans = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    missile_trans.set_world_position(missile_pos);

    auto& voxel_renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    auto ref = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(enemy.missile_voxel_object);
    voxel_renderer.resource = ref;

    auto& voxel_body = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    voxel_body.layer = enemy.projectile_layer;
    voxel_body.type = tmt::VoxelBody::STATIC;

    // set random offset
    offset = glm::vec3(
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness
    );

    // initial missile rotation
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(enemy.walkable_asteroid);
    if (!nav_mesh.nav_nodes_valid()) return;
    auto ground_up = (*nav_mesh.nodes_mesh)[nav_mesh.find_closest_node(enemy_pos)].normal;
    glm::vec3 forward = glm::normalize(ground_up);
    glm::vec3 up = (glm::abs(glm::dot(forward, ground_up)) < 0.99f) ? ground_up : glm::vec3(0.0f, 1.0f, 0.0f);
    missile_trans.set_world_rotation(glm::quatLookAt(forward, up));
}

void game::Missile::update(const tmt::FrameData& time) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    auto& voxel_body = tmt::engine.ecs.get_component<tmt::VoxelBody>(entity);
    // voxel_body.layer = 0;
    if (life_time > enemy.life_time) {
        explode();
        return;
    }
    life_time += time.delta_time;

    auto& position = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto player_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy.player).get_world_position();
    player_pos += enemy.target_offset;
    auto enemy_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).get_world_position();
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(enemy.walkable_asteroid);
    if (!nav_mesh.nav_nodes_valid()) return;
    auto ground_up = (*nav_mesh.nodes_mesh)[nav_mesh.find_closest_node(enemy_pos)].normal;

    // fades from full launch down to zero
    float launch_t = glm::clamp((life_time / enemy.stop_launching_after), 0.0f, 1.0f);
    glm::vec3 launch_contribution = glm::mix(ground_up * enemy.launch_speed, glm::vec3(0.0f), launch_t);

    // fades from zero up to full homing speed
    glm::vec3 home_contribution(0.0f);
    if (life_time > enemy.start_homing_after) {
        float max_home_time = enemy.life_time - enemy.start_homing_after;
        float home_time = life_time - enemy.start_homing_after;
        float home_t = glm::clamp(home_time / max_home_time, 0.0f, 1.0f);

        glm::vec3 to_player = player_pos - position.get_world_position();
        if (glm::dot(to_player, to_player) > 0.0001f) {
            home_contribution = glm::normalize(to_player) * enemy.home_speed * home_t;
        }
    }

    // slow down homing towards end of life
    if (life_time > enemy.slow_homing_accuracy_after && glm::dot(velocity, velocity) > 0.0001f) {
        float max_slowing_time = enemy.life_time - enemy.slow_homing_accuracy_after;
        float slowing_time = life_time - enemy.slow_homing_accuracy_after;
        float slow_t = glm::clamp(slowing_time / max_slowing_time, 0.0f, 1.0f);
        float home_contribution_length = glm::length(home_contribution);
        home_contribution = glm::mix(home_contribution, velocity, slow_t);
        home_contribution = glm::normalize(home_contribution) * home_contribution_length;
    }

    auto added_vel = launch_contribution + home_contribution;
    float length = glm::length(added_vel);
    if (length > 0.0001f && life_time < enemy.start_homing_after) {
        added_vel += offset;
        added_vel = glm::normalize(added_vel) * length;
    }
    velocity += added_vel;
    velocity *= 0.7f;
    position.translate(velocity * time.delta_time);

    // missile rotation
    if (glm::dot(velocity, velocity) > 0.0001f) {
        glm::vec3 forward = glm::normalize(velocity);
        glm::vec3 up = (glm::abs(glm::dot(forward, ground_up)) < 0.99f) ? ground_up : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::quat target_rot = glm::quatLookAt(forward, up);
        glm::quat current_rot = position.get_world_rotation();
        glm::quat rot = glm::slerp(current_rot, target_rot, enemy.missile_rotation_speed * time.delta_time);
        position.set_world_rotation(rot);
    }

    // collision check
    // const tmt::Ray ray_cast = tmt::Ray(previous_position, glm::normalize(delta));
    // const tmt::Hit hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray_cast, layer_mask);

    const tmt::Ray ray_cast = tmt::Ray(position.get_world_position(), glm::normalize(velocity));

    const tmt::Hit hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray_cast, enemy.enemy_projectile_mask);

    // TODO this is very dumb code that should be updated when we have proper raycasts check
    if (hit.entity != entt::null && hit.distance < 0.1f || glm::distance(player_pos, position.get_world_position()) < enemy.missile_explosion_radius) {
        explode();
    }
}

void game::Missile::end() {}

void game::Missile::explode() {
    auto& enemy = tmt::engine.ecs.get_component<MediumEnemy>(enemy_entity);

    auto* audio_emitter = tmt::engine.ecs.try_get_component<tmt::AudioEmitter>(enemy_entity);
    if (audio_emitter != nullptr) {
        const tmt::AudioInstance3D instance = audio_emitter->play(enemy.sounds.sound_missile_explosion);
        instance.set_maximum_distance(enemy.aggro_range * 1.25f);  // Multiply be 1.25f to ensure the player can hear it even when at the edge of the range
    }

    enemy.ore_manager->initiate_thermite_explosion(entity, glm::uvec3(0, 0, 0));

    auto& missile_trans = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& voxel_body = tmt::engine.ecs.get_component<tmt::VoxelBody>(entity);
    voxel_body.layer = 0;
    voxel_body.type = tmt::VoxelBody::DYNAMIC;
    voxel_body.gravity = 0.0f;
    voxel_body.velocity = velocity;
    voxel_body.position = missile_trans.get_world_position();
    voxel_body.rotation = missile_trans.get_world_rotation();

    auto& renderer = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(entity);

    tmt::engine.ecs.systems.try_get<tmt::Physics>()->recalculate_physics_data(voxel_body, *renderer.resource.resource);

    tmt::engine.ecs.remove_component<Missile>(entity);
}
