#include "explode.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/emitter.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/animation/rig_model.hpp"

#include "engine/systems/ai/steering/steering_system.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"

#include "../gameplay_functionality_components/player.hpp"
#include "../gameplay_functionality_components/weapon_and_tool_components/explosion.hpp"
#include "../managers/ore_manager.hpp"
#include "../gameplay_functionality_components/enemy_components/small_enemy.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace game {

void Explode::on_start(tmt::Entity agent) {
    auto& ecs = tmt::engine.ecs;
    auto& registry = ecs.get_registry();

    if (!registry.any_of<SteeringAgent>(agent)) {
        registry.emplace<SteeringAgent>(agent);
    }

    auto* steering = ecs.systems.try_get<tmt::SteeringSystem>();

    if (!steering) {
        tmt::Log::warn("Steering system not active.");
        return;
    }

    if (!ecs.valid(player_entity)) {
        player_entity = ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game
    }

    live_charge_time = 0.0f;
    exploded = false;
}

void Explode::on_tick(tmt::Entity agent, float dt) {
    auto& ecs = tmt::engine.ecs;

    live_charge_time += dt;

    auto& registry = ecs.get_registry();
    auto& steering_agent = registry.get<SteeringAgent>(agent);

    auto& transform = registry.get<tmt::Transform>(agent);

    glm::vec3 enemy_pos = transform.get_world_position();
    glm::vec3 player_pos = ecs.get_component<tmt::Transform>(player_entity).get_world_position();

    // direction to player
    glm::vec3 dir = player_pos - enemy_pos;
    dir.y = 0.0f;

    if (glm::length2(dir) > 0.0001f) {
        dir = glm::normalize(dir);

        glm::quat target_rot = glm::quatLookAt(-dir, glm::vec3(0, 1, 0));

        // optional model correction (VERY likely needed in your engine)
        target_rot *= glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0));

        // choose correct rotation owner
        if (registry.any_of<tmt::VoxelBody>(agent)) {
            auto& body = registry.get<tmt::VoxelBody>(agent);

            glm::quat current_rot = body.rotation;
            float turn_speed = 6.0f;

            body.rotation = glm::slerp(current_rot, target_rot, turn_speed * dt);
        } else {
            glm::quat current_rot = transform.get_world_rotation();
            float turn_speed = 6.0f;

            transform.set_world_rotation(glm::slerp(current_rot, target_rot, turn_speed * dt));
        }
    }

    auto* small_enemy = ecs.try_get_component<SmallEnemy>(agent);

    // try get small enemy component, and the core entity
    if (!small_enemy || !ecs.valid(small_enemy->thermite)) {
        tmt::Log::warn("Enemy doesnt have small enemy component OR a core assigned.");
        return;  // nothing to explode
    }

    if (live_charge_time >= small_enemy->logic_paramaters.charge_time && !exploded) {
        // try get small enemy component, and the explosion entity, to get the particle emitter
        if (tmt::engine.ecs.valid(small_enemy->explosion)) {
            auto& particles = tmt::engine.ecs.get_component<tmt::ParticleEmitter>(small_enemy->explosion);

            particles.should_burst = true;
        }

        tmt::Entity thermite = small_enemy->thermite;

        // get core entity transform world position
        auto& core_transform = ecs.get_component<tmt::Transform>(thermite);
        glm::vec3 explosion_center = core_transform.get_world_position();

        // --- create explosion entity ---
        tmt::Entity explosion_entity = ecs.create_entity();

        // Transform
        auto& explosion_transform = ecs.get_component<tmt::Transform>(explosion_entity);
        explosion_transform.set_world_position(explosion_center);

        // Explosion component
        auto& explosion = ecs.add_component<Explosion>(explosion_entity);

        // Copy params from agent
        explosion.param = small_enemy->explosion_parameters;

        // Get everything (with voxel body) within radius and push away
        float radius = small_enemy->logic_paramaters.push_radius;
        float force = small_enemy->logic_paramaters.explosion_force;

        for (auto [entity, body, transform] : ecs.view<tmt::VoxelBody, tmt::Transform>().each()) {
            glm::vec3 pos = transform.get_world_position();
            glm::vec3 dir = pos - explosion_center;

            float dist = glm::length(dir);

            if (dist <= radius && dist > 0.001f) {
                glm::vec3 normal = glm::normalize(dir);

                // falloff, less force further away
                float strength = 1.0f - (dist / radius);

                glm::vec3 explosion_velocity = normal * force * strength;

                // apply an impulse
                body.velocity += explosion_velocity;
            }
        }

        if (ecs.valid(player_entity)) {
            auto& player_transform = ecs.get_component<tmt::Transform>(player_entity);
            auto& player = ecs.get_component<Player>(player_entity);

            glm::vec3 player_pos = player_transform.get_world_position();
            glm::vec3 dir = player_pos - explosion_center;

            float dist = glm::length(dir);

            if (dist <= radius && dist > 0.001f) {
                player.health.value -= small_enemy->logic_paramaters.explosion_damage;

                glm::vec3 normal = glm::normalize(dir);

                // same falloff as physics bodies
                float strength = 1.0f - (dist / radius);

                // push player
                float player_force = small_enemy->logic_paramaters.explosion_force;
                player.apply_impulse(normal, player_force * strength);
            }
        }

        std::set<tmt::Entity> children = transform.get_all_children();

        for (tmt::Entity child : children) {
            if (!ecs.valid(child)) continue;

            if (ecs.try_get_component<tmt::RigModel>(child)) {
                ecs.remove_component<tmt::RigModel>(child);
            }
        }

        // destroy core entity & explosion
        ecs.destroy_entity(thermite);

        // remove GOAP so it doesn't keep acting
        ecs.remove_component<tmt::GoapAgent>(agent);

        exploded = true;
    }
}

bool Explode::is_done(tmt::Entity agent) const {
    return exploded;
}

void Explode::on_finished(tmt::Entity agent) {}

void Explode::on_interrupt(tmt::Entity agent) {}

}  // namespace game
