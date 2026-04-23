#include "random_prefab_spawner.hpp"

#include "engine/systems/physics/physics_system.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/tools/prefab_helper.hpp"

void game::RandomPrefabSpawner::start() {
    respawn_timer = respawn_time;
    spawn_random_prefab();
}

void game::RandomPrefabSpawner::update(const tmt::FrameData& time) {
    if (!tmt::engine.ecs.valid(spawned_entity)) {
        // spawned entity is invalid -> has been destroyed
        // set to entt::null to ensure that no other entity with the same ID makes this valid again
        spawned_entity = entt::null;
    }
    // spawned entity has either been destroyed or was not spawned in (can happen when prefabs get set during runtime)
    if (spawned_entity == entt::null) {
        respawn_timer += time.delta_time;
        if (respawn_timer >= respawn_time) {
            if (spawn_count < spawn_limit || spawn_limit == 0) {
                if (re_randomize) {
                    spawn_random_prefab();
                } else {
                    spawn_prefab(last_spawned_prefab);
                }
            }
        }
    }
}

void game::RandomPrefabSpawner::end() {
}

void game::RandomPrefabSpawner::spawn_random_prefab() {
    if (prefabs.empty()) {
        tmt::Log::error("No prefabs found on prefab spawner component, entity: {}", entity);
        return;
    }
    std::uniform_int_distribution<> dis(0, static_cast<int>(prefabs.size()) - 1);

    int random_index = dis(rng);

    // Spawn the randomly selected prefab
    spawn_prefab(prefabs[random_index]);
}

void game::RandomPrefabSpawner::spawn_prefab(const tmt::ResourceRef<tmt::Json>& prefab) {
    auto instantiated = tmt::PrefabHelper::instantiate_prefab(prefab->file_location);
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated);
    auto& transform_parent = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto voxel_body = tmt::engine.ecs.try_get_component<tmt::VoxelBody>(instantiated);
    if (voxel_body) {
        tmt::Physics::set_position(*voxel_body, transform_parent.get_world_position());
    }
    transform.set_world_position(transform_parent.get_world_position());

    spawned_entity = instantiated;
    last_spawned_prefab = prefab;
    respawn_timer = 0.0f;
    spawn_count++;
}