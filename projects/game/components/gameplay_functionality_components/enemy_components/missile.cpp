#include "missile.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/emitter.hpp"

#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/tools/prefab_helper.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"

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
    voxel_body.type = tmt::VoxelBody::DYNAMIC;
    voxel_body.gravity = 0.0f;

    // set random offset
    offset = glm::vec3(
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness
    );

    // initial missile rotation
    auto ground_up = -tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).get_up();
    glm::vec3 forward = glm::normalize(ground_up);
    glm::vec3 up = (glm::abs(glm::dot(forward, ground_up)) < 0.99f) ? ground_up : glm::vec3(0.0f, -1.0f, 0.0f);
    missile_trans.set_world_rotation(glm::quatLookAt(forward, up));

    // instantiate vfx prefab
    auto explosion_prefab = enemy.missile_vfx_prefab;
    vfx_entity = tmt::PrefabHelper::instantiate_prefab(explosion_prefab->file_location);
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(vfx_entity);
    transform.set_parent(entity);
    transform.set_local_rotation(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
    tmt::ParticleEmitter* emitter_component = tmt::engine.ecs.try_get_component<tmt::ParticleEmitter>(vfx_entity);
    if (!emitter_component) {
        tmt::Log::error("Cant spawn particle on emitter, check prefab on ore manager component on entity: {}", entity);
        return;
    }
    emitter_component->active = true;
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

    //auto& position = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto player_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy.player).get_world_position();
    player_pos += enemy.target_offset;
    auto enemy_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).get_world_position();
    auto ground_up = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).get_up();

    // collision check
    // const tmt::Ray ray_cast = tmt::Ray(previous_position, glm::normalize(delta));
    // const tmt::Hit hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray_cast, layer_mask);
    
    const tmt::Ray ray_cast = tmt::Ray(voxel_body.position, glm::normalize(voxel_body.velocity));

    game::LayerMask mask;
    if (has_been_close_to_player) {
        mask = enemy.projectile_mask;
    } else {
        mask = enemy.enemy_projectile_mask;
    }
    const tmt::Hit hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray_cast, mask);

    // check for collision with player or if within explosion radius, then explode
    if (hit.distance < enemy.missile_explosion_radius || glm::distance(player_pos, voxel_body.position) < enemy.missile_explosion_radius) {
        tmt::engine.ecs.get_component<game::Player>(enemy.player).apply_impulse(voxel_body.velocity, 4.0f);
        explode();
    }

    if ((hit.distance > enemy.missile_explosion_radius * 4.0f && glm::distance(player_pos, voxel_body.position) > enemy.missile_explosion_radius * 4.0f) &&
        !has_been_close_to_player) {
        // fades from full launch down to zero
        float launch_t = glm::clamp((life_time / enemy.stop_launching_after), 0.0f, 1.0f);
        glm::vec3 launch_contribution = glm::mix(ground_up * enemy.launch_speed, glm::vec3(0.0f), launch_t);

        // fades from zero up to full homing speed
        glm::vec3 home_contribution(0.0f);
        if (life_time > enemy.start_homing_after) {
            float max_home_time = enemy.life_time - enemy.start_homing_after;
            float home_time = life_time - enemy.start_homing_after;
            float home_t = glm::clamp(home_time / max_home_time, 0.0f, 1.0f);

            glm::vec3 to_player = player_pos - voxel_body.position;
            if (glm::dot(to_player, to_player) > 0.0001f) {
                home_contribution = glm::normalize(to_player) * enemy.home_speed * home_t;
            }
        }

        // slow down homing towards end of life
        if (life_time > enemy.slow_homing_accuracy_after && glm::dot(voxel_body.velocity, voxel_body.velocity) > 0.0001f) {
            float max_slowing_time = enemy.life_time - enemy.slow_homing_accuracy_after;
            float slowing_time = life_time - enemy.slow_homing_accuracy_after;
            float slow_t = glm::clamp(slowing_time / max_slowing_time, 0.0f, 1.0f);
            float home_contribution_length = glm::length(home_contribution);
            home_contribution = glm::mix(home_contribution, voxel_body.velocity, slow_t);
            home_contribution = glm::normalize(home_contribution) * home_contribution_length;
        }

        auto added_vel = launch_contribution + home_contribution;
        float length = glm::length(added_vel);
        if (length > 0.0001f && life_time < enemy.start_homing_after) {
            added_vel += offset;
            added_vel = glm::normalize(added_vel) * length;
        }
        voxel_body.velocity += added_vel;
        voxel_body.velocity *= 0.7f;
        // position.translate(voxel_body.velocity * time.delta_time);
    } else {
        has_been_close_to_player = true;
    }

    // missile rotation
    if (glm::dot(voxel_body.velocity, voxel_body.velocity) > 0.0001f) {
        glm::vec3 forward = glm::normalize(-voxel_body.velocity);
        glm::vec3 up = (glm::abs(glm::dot(forward, ground_up)) < 0.99f) ? ground_up : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::quat target_rot = glm::quatLookAt(forward, up);
        glm::quat current_rot = voxel_body.rotation;
        float rot_speed = enemy.missile_rotation_speed;
        if (has_been_close_to_player) rot_speed *= 5.0f; 
        voxel_body.rotation = glm::slerp(current_rot, target_rot, rot_speed * time.delta_time);
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
    // delete vfx entity
    if (tmt::engine.ecs.valid(vfx_entity)) {
        tmt::engine.ecs.destroy_entity(vfx_entity);
    }

    enemy.ore_manager->initiate_thermite_explosion(entity, glm::uvec3(1, 1, 1));

    //auto& missile_trans = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //auto& voxel_body = tmt::engine.ecs.get_component<tmt::VoxelBody>(entity);
    //voxel_body.layer = 0;
    //voxel_body.type = tmt::VoxelBody::DYNAMIC;
    //voxel_body.gravity = 0.0f;
    //// voxel_body.velocity = voxel_body.velocity;
    //voxel_body.position = missile_trans.get_world_position();
    //voxel_body.rotation = missile_trans.get_world_rotation();

    //auto& renderer = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(entity);

    //tmt::engine.ecs.systems.try_get<tmt::Physics>()->recalculate_physics_data(voxel_body, *renderer.resource.resource);

    tmt::engine.ecs.remove_component<Missile>(entity);
}
