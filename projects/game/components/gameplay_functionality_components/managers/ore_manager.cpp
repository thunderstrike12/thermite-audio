#include "ore_manager.hpp"

#include <queue>

#include "engine/core/components/voxel_renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "projects/game/components/player.hpp"

namespace game {

void OreManager::start() {
    auto player_view = tmt::engine.ecs.view<Player>();

    if (!player_view.empty()) {
        player_entity = player_view.front().entity;
    } else {
        tmt::Log::warn("No player found in scene, cannot set player for ore manager.");
    }
}

void OreManager::update(const tmt::FrameData& time) {
    if (thermite_ore_explosion_cooldown_timer >= thermite_ore_settings.cooldown_explosion) {
        new_positions_to_explode.clear();
        for (auto explosion_center : positions_to_explode) {
            for (auto hit_entity : hit_entities) {
                process_thermite_ore_explosion(hit_entity, explosion_center);
            }
        }
        // update explosion list
        positions_to_explode = new_positions_to_explode;
        thermite_ore_explosion_cooldown_timer = 0.0f;
    }
    thermite_ore_explosion_cooldown_timer += time.delta_time;
}

void OreManager::end() {}

void OreManager::initiate_thermite_explosion(tmt::Entity voxel_entity, glm::uvec3 voxel_position) {
    hit_entities.insert(voxel_entity);
    positions_to_explode.insert(voxel_position);
}

void OreManager::process_thermite_ore_explosion(tmt::Entity voxel_entity, glm::uvec3 explosion_center) {  // explosion center is uint voxel position relative to entity transform
    auto vox_renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(voxel_entity);
    auto& vox_entity_transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel_entity);
    int voxel_level_radius = static_cast<int>(thermite_ore_settings.radius_explosion * VOXELS_PER_UNIT);
    int cx = static_cast<int>(explosion_center.x);
    int cy = static_cast<int>(explosion_center.y);
    int cz = static_cast<int>(explosion_center.z);

    glm::vec3 explosion_center_local_pos = glm::vec3(explosion_center) / static_cast<float>(VOXELS_PER_UNIT);
    glm::vec3 radiusexplosion_world_pos = glm::vec3(vox_entity_transform.get_world_matrix() * glm::vec4(explosion_center_local_pos, 1.0f));

    // check if player is within explosion

    if (glm::length(tmt::engine.ecs.get_component<tmt::Transform>(player_entity).get_world_position() - radiusexplosion_world_pos) <= thermite_ore_settings.radius_explosion) {
        tmt::engine.ecs.get_component<Player>(player_entity).health.value -= thermite_ore_settings.damage_explosion;
    }

    // remove initial voxel (explosion center)
    vox_renderer->resource->blas->remove_voxel(cx, cy, cz);
    vox_renderer->resource->set_dirty();

    for (int x = cx - voxel_level_radius; x <= cx + voxel_level_radius; x++) {
        for (int y = cy - voxel_level_radius; y <= cy + voxel_level_radius; y++) {
            for (int z = cz - voxel_level_radius; z <= cz + voxel_level_radius; z++) {
                // distances
                int dx = x - cx;
                int dy = y - cy;
                int dz = z - cz;
                int distance = static_cast<int>(std::sqrt(dx * dx + dy * dy + dz * dz));

                if (distance <= voxel_level_radius) {
                    auto curr_vox = vox_renderer->resource->blas->get_voxel(x, y, z);
                    // add if is thermite to new_positions_to_explode
                    if (curr_vox && curr_vox->type == tmt::Material::Type::THERMITE) {
                        new_positions_to_explode.insert(glm::vec3(x, y, z));
                    }
                    // delete
                    vox_renderer->resource->blas->remove_voxel(x, y, z);
                    vox_renderer->resource->set_dirty();
                }
            }
        }
    }
}

}  // namespace game