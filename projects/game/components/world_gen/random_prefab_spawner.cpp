#include "random_prefab_spawner.hpp"

#include "engine/systems/physics/physics_system.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/tools/prefab_helper.hpp"

void game::RandomPrefabSpawner::start() {
    if (prefabs.empty()) {
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(prefabs.size()) - 1);

    int random_index = dis(gen);

    // Spawn the randomly selected prefab
    spawn_prefab(prefabs[random_index]);
}

void game::RandomPrefabSpawner::update(const tmt::FrameData& time) {}

void game::RandomPrefabSpawner::end() {}

void game::RandomPrefabSpawner::spawn_prefab(const tmt::ResourceRef<tmt::Json>& prefab) const {
    auto instantiated = tmt::PrefabHelper::instantiate_prefab(prefab->file_location);
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated);
    auto& transform_parent = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto voxel_body = tmt::engine.ecs.try_get_component<tmt::VoxelBody>(instantiated);
    if (voxel_body) {
        tmt::Physics::set_position(*voxel_body, transform_parent.get_world_position());
    }
    transform.set_world_position(transform_parent.get_world_position());

    // transform.set_world_rotation(transform_parent.get_world_rotation());
}