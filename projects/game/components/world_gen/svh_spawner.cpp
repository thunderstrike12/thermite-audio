#include "svh_spawner.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/physics/components/destructable.hpp"
#include "engine/core/components/voxel_renderer.hpp"

namespace game {

void SVHSpawner::start() {
    if (!scene_to_spawn) {
        tmt::Log::error("No scene assigned to SVHSpawner on entity {}. Please assign a scene to spawn.", entity);
        return;
    }
    const std::vector<tmt::Entity> roots = scene_to_spawn->instantiate_entities(entity, false);
    if (roots.empty()) {
        tmt::Log::error("Failed to spawn scene from SVHSpawner on entity {}. The scene did not instantiate any entities.", entity);
    }

    auto& root_transform = tmt::engine.ecs.get_component<tmt::Transform>(roots.front());
    root_transform.set_local_position({0.f, 0.f, 0.f});

    for (const tmt::Entity root : roots) {
        add_required(root);
        init_children(root);
    }
}

void SVHSpawner::update(const tmt::FrameData&) {}

void SVHSpawner::end() {}

void SVHSpawner::init_children(const tmt::Entity parent) {
    const auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(parent);
    const auto children = transform.get_children();
    for (const auto child : children) {
        add_required(child);
        init_children(child);
    }
}

void SVHSpawner::add_required(const entt::entity child) {
    auto* voxel_renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(child);
    if (voxel_renderer == nullptr) return;
    voxel_renderer->distance_culling = needs_distance_culling;

    /* Add voxel body */
    auto& voxel_body = tmt::engine.ecs.add_component<tmt::VoxelBody>(child);
    voxel_body.type = tmt::VoxelBody::STATIC;

    tmt::engine.ecs.add_component<tmt::Destructible>(child);
}

}  // namespace game