#include "ore_manager.hpp"

#include <queue>

#include "engine/core/components/voxel_renderer.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"

namespace game {

void OreManager::start() {
    auto player_view = tmt::engine.ecs.view<Player>();

    if (!player_view.empty()) {
        player_entity = player_view.front().entity;
    } else {
        tmt::Log::warn("No player found in scene, cannot set player for ore manager.");
    }

    auto ore_prop_view = tmt::engine.ecs.view<OreProperties>();
    if (!ore_prop_view.empty()) {
        ore_properties = tmt::engine.ecs.try_get_component<OreProperties>(ore_prop_view.front().entity);
        ore_database = ore_properties->ores;
    } else {
        tmt::Log::warn("No ore properties found, needed for ore toughness checks");
        return;
    }
}

void OreManager::update(const tmt::FrameData& time) {
    if (thermite_ore_explosion_cooldown_timer >= thermite_ore_settings.cooldown_explosion) {
        new_thermite_to_explode.clear();
        for (auto& thermite_voxel : thermite_to_explode) {
            process_thermite_ore_explosion(thermite_voxel.first, thermite_voxel.second);
        }
        // update explosion list
        thermite_to_explode = new_thermite_to_explode;
        thermite_ore_explosion_cooldown_timer = 0.0f;
    }
    thermite_ore_explosion_cooldown_timer += time.delta_time;
}

void OreManager::end() {}

void OreManager::initiate_thermite_explosion(tmt::Entity voxel_entity, glm::uvec3 voxel_position) {
    thermite_to_explode.insert(std::make_pair(voxel_entity, voxel_position));
}

void OreManager::process_thermite_ore_explosion(tmt::Entity voxel_entity, glm::uvec3 explosion_center) {  // explosion center is uint voxel position relative to entity transform
    auto origin_entity_world_pos = tmt::engine.ecs.get_component<tmt::Transform>(voxel_entity).get_world_position();
    auto vox_renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(voxel_entity);
    auto& vox_entity_transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel_entity);
    int cx = static_cast<int>(explosion_center.x);
    int cy = static_cast<int>(explosion_center.y);
    int cz = static_cast<int>(explosion_center.z);

    glm::vec3 explosion_center_local_pos = glm::vec3(explosion_center) / static_cast<float>(VOXELS_PER_UNIT);
    glm::vec3 explosion_world_pos = glm::vec3(vox_entity_transform.get_world_matrix() * glm::vec4(explosion_center_local_pos, 1.0f));

    // check if player is within explosion
    if (glm::length(tmt::engine.ecs.get_component<tmt::Transform>(player_entity).get_world_position() - explosion_world_pos) <= thermite_ore_settings.radius_explosion) {
        tmt::engine.ecs.get_component<Player>(player_entity).health.value -= thermite_ore_settings.damage_explosion;
    }
    // remove initial voxel (explosion center)
    vox_renderer->resource->blas->remove_voxel(cx, cy, cz);
    vox_renderer->resource->set_dirty();

    // refactor: use sphere check instead
    auto sphere_check_result = tmt::engine.ecs.systems.get<tmt::Physics>().overlap_sphere(explosion_world_pos, thermite_ore_settings.radius_explosion, thermite_ore_settings.layer_mask);

    for (auto& result_pair : sphere_check_result) {
        auto result_entity_world_pos = tmt::engine.ecs.get_component<tmt::Transform>(result_pair.first).get_world_position();
        auto result_entity_local_pos = tmt::engine.ecs.get_component<tmt::Transform>(result_pair.first).get_local_position();
        // Guard for voxel renderer
        if (auto result_vox_renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(result_pair.first)) {
            // Loop over resulting voxels
            for (auto& result_voxel : result_pair.second) {
                auto* curr_vox_material = result_vox_renderer->resource->blas->get_voxel(result_voxel.second.x, result_voxel.second.y, result_voxel.second.z);
                // Skip voxels without a material
                if (!curr_vox_material) continue;
                auto curr_vox_type = curr_vox_material->type;
                if (curr_vox_type == tmt::Material::Type::THERMITE) {
                    // Add to " new thermite to explode"
                    new_thermite_to_explode.insert(std::make_pair(result_pair.first, result_voxel.second));
                }
                // Guard for ore properties
                if (ore_properties) {
                    // Check for toughness
                    auto curr_vox_toughness = ore_database.at(curr_vox_type).toughness;
                    if (curr_vox_toughness <= thermite_ore_settings.explosion_strength) {
                        result_vox_renderer->resource->blas->remove_voxel(result_voxel.second.x, result_voxel.second.y, result_voxel.second.z);
                        result_vox_renderer->resource->set_dirty();
                    }
                } else {
                    tmt::Log::warn("No ore properties found, will explode and remove all voxels on thermite explosion");
                    result_vox_renderer->resource->blas->remove_voxel(result_voxel.second.x, result_voxel.second.y, result_voxel.second.z);
                    result_vox_renderer->resource->set_dirty();
                }
            }
        }
    }
}

}  // namespace game