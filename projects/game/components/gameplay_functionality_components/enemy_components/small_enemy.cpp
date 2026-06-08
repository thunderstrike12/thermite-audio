#include "small_enemy.hpp"

#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "engine\core\components\voxel_renderer.hpp"
#include "engine/core/components/audio_emitter.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/polyline.hpp"
#include "engine/steam/achievements.hpp"
#include "engine/steam/steam_api.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/tools/random.hpp"

namespace game {

void SmallEnemy::start() {
    auto& ecs = tmt::engine.ecs;

    // Update each agent with a SteeringAgent component
    ecs.view<SteeringAgent>().each([&](tmt::Entity agent, SteeringAgent& steering) {
        steering.min_explosion_range = logic_paramaters.min_explosion_range;
        steering.activation_range = logic_paramaters.activation_range;

        steering.arrive_radius = movement_paramaters.arrive_radius;
        steering.max_force = movement_paramaters.max_force;
        steering.max_speed = movement_paramaters.max_speed;
        steering.wander_radius_limit = movement_paramaters.wander_radius_limit;
    });

    // Cache initial voxel count of the core
    auto resource = ecs.get_component<tmt::VoxelRenderer>(core).resource;

    core_voxels = resource->blas->voxel_count - resource->blas->voxels_wasted;
}

void SmallEnemy::update(const tmt::FrameData& time) {
    if (core_destroyed) return;

    auto& ecs = tmt::engine.ecs;

    // Get current voxel count
    auto resource = ecs.get_component<tmt::VoxelRenderer>(core).resource;

    uint32_t current_core_voxels = resource->blas->voxel_count - resource->blas->voxels_wasted;

    // If voxel count changed, core was damaged/destroyed
    if (current_core_voxels != core_voxels) {
        core_destroyed = true;
        return;
    }

    // Handle playing random chatter sound.
    auto* audio_emitter = tmt::engine.ecs.try_get_component<tmt::AudioEmitter>(entity);
    if (audio_emitter) {
        // Set up the random number generator.
        static std::random_device random_device;
        static std::mt19937 random_generator(random_device());
        constexpr float MAX_RANDOM_VALUE = static_cast<float>(std::mt19937::max());

        // Play the chatter sound effect again after a random time (within range).
        if (time.elapsed_time >= next_chatter_time) {
            audio_emitter->play(sound_parameters.idle_chatter_audio);

            const float lerp = static_cast<float>(random_generator()) / MAX_RANDOM_VALUE;
            const float interval = glm::mix(sound_parameters.chatter_play_intervals.x, sound_parameters.chatter_play_intervals.x, lerp);

            next_chatter_time = time.elapsed_time + interval;
        }
    }
}

void SmallEnemy::die(tmt::Entity agent) {
    tmt::engine.steam.achievement->stats.enemy_killed_count++;
    tmt::engine.steam.achievement->store_stats = true;
    auto& ecs = tmt::engine.ecs;
    auto& registry = ecs.get_registry();

    auto& transform = registry.get<tmt::Transform>(agent);

    std::set<tmt::Entity> children = transform.get_all_children();

    for (tmt::Entity child : children) {
        if (!ecs.valid(child)) continue;

        if (ecs.has_component<tmt::RigModel>(child)) {
            ecs.remove_component<tmt::RigModel>(child);
        }

        if (ecs.has_component<tmt::VoxelBody>(child) == false) continue;

        if (auto* renderer = ecs.try_get_component<tmt::VoxelRenderer>(child)) {
            tmt::VoxelBody* body = ecs.try_get_component<tmt::VoxelBody>(child);

            if (body->type == tmt::VoxelBody::STATIC) {
                // Preserve current world transform
                auto& child_transform = registry.get<tmt::Transform>(child);
                auto pos = child_transform.get_world_position();
                auto rot = child_transform.get_world_rotation();

                // Force sync physics transform
                body->type = tmt::VoxelBody::DYNAMIC;
                body->position = pos;
                body->rotation = rot;

                // small random velocity forward
                glm::vec3 forward = transform.get_forward();
                body->velocity += -forward * Random::rand_range(0.5f, 2.5f);
                body->angular_velocity += glm::vec3(Random::rand_range(-1.0f, 1.0f), Random::rand_range(-1.0f, 1.0f), Random::rand_range(-1.0f, 1.0f));

                tmt::Physics::initialize_voxel_body(*body, *renderer->resource.resource.get());
            }
        }
    }

    // If the audio emitter exists and is playing sounds it should stop the sounds.
    auto* audio_emitter = tmt::engine.ecs.try_get_component<tmt::AudioEmitter>(entity);
    if (audio_emitter != nullptr) {
        audio_emitter->stop_instances();
    }
}

void SmallEnemy::play_aggro_sound() const {
    // Check if the agent has an audio emitter.
    auto* audio_emitter = tmt::engine.ecs.try_get_component<tmt::AudioEmitter>(entity);
    if (audio_emitter == nullptr) return;

    const auto& small_enemy = tmt::engine.ecs.get_component<SmallEnemy>(entity);

    const tmt::AudioInstance3D instance = audio_emitter->play(small_enemy.sound_parameters.aggroed_audio);
    instance.set_maximum_distance(small_enemy.logic_paramaters.activation_range * 1.25f);  // Multiply be 1.25f to ensure the player can hear it even when at the edge of the range.
}

}  // namespace game
