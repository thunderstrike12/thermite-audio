#include "ore_manager.hpp"

#include <queue>

#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/emitter.hpp"
#include "engine/systems/physics/destruction_system.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/tools/prefab_helper.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"
#include <engine/tools/random.hpp>
#include "engine/core/polyline.hpp"

namespace game {

void OreManager::start() {
    auto player_view = tmt::engine.ecs.view<Player>(entt::exclude_t {});

    if (!player_view.empty()) {
        player_entity = player_view.front().entity;
    } else {
        tmt::Log::warn("No player found in scene, cannot set player for ore manager.");
    }

    auto ore_prop_view = tmt::engine.ecs.view<OreProperties>();
    if (!ore_prop_view.empty()) {
        ore_properties_entity = ore_prop_view.front().entity;
        auto* ore_properties = tmt::engine.ecs.try_get_component<OreProperties>(ore_prop_view.front().entity);
        ore_database = ore_properties->ores;
    } else {
        tmt::Log::warn("No ore properties found, needed for ore toughness checks");
    }
}

void OreManager::update(const tmt::FrameData& time) {
    // explosion vfx handling
    std::unordered_set<tmt::Entity> destroyed_emitters;
    for (std::pair<const tmt::Entity, float>& emitter_entity : emitter_lifetime_table) {
        if (emitter_entity.second < 0.0f) {
            tmt::engine.ecs.destroy_entity(emitter_entity.first);
            destroyed_emitters.insert(emitter_entity.first);
        } else {
            emitter_entity.second -= time.delta_time;
        }
    }
    for (auto emitter_entity : destroyed_emitters) {
        emitter_lifetime_table.erase(emitter_entity);
    }

    // explosion logic handling
    if (thermite_ore_explosion_cooldown_timer >= thermite_ore_settings.cooldown_explosion) {
        new_thermite_to_explode.clear();
        for (auto& thermite_voxel : thermite_to_explode) {
            if (tmt::engine.ecs.valid(thermite_voxel.first)) {
                process_thermite_ore_explosion(thermite_voxel.first, thermite_voxel.second);
            }
        }
        // update explosion list
        thermite_to_explode = new_thermite_to_explode;
        thermite_ore_explosion_cooldown_timer = 0.0f;
        tmt::engine.ecs.get_dispatcher().trigger<ThermiteExplosionEvent>();
    }
    thermite_ore_explosion_cooldown_timer += time.delta_time;
}

void OreManager::end() {}

void OreManager::initiate_thermite_explosion(tmt::Entity voxel_entity, glm::uvec3 voxel_position) {
    thermite_to_explode.insert(std::make_pair(voxel_entity, voxel_position));
}

void OreManager::process_thermite_ore_explosion(tmt::Entity voxel_entity, glm::uvec3 explosion_center) {
    // explosion center is uint voxel position relative to entity transform
    auto origin_entity_world_pos = tmt::engine.ecs.get_component<tmt::Transform>(voxel_entity).get_world_position();
    auto vox_renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(voxel_entity);
    if (!vox_renderer) return;
    auto& vox_entity_transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel_entity);
    int cx = static_cast<int>(explosion_center.x);
    int cy = static_cast<int>(explosion_center.y);
    int cz = static_cast<int>(explosion_center.z);

    glm::vec3 explosion_center_local_pos = (glm::vec3(explosion_center) - glm::vec3(vox_renderer->resource->size) / 2.0f) * UNITS_PER_VOXEL;
    auto explosion_world_pos = glm::vec3(vox_entity_transform.get_world_matrix() * glm::vec4(explosion_center_local_pos, 1.0f));
    auto* curr_vox_material = vox_renderer->resource->blas->get_voxel(explosion_center.x, explosion_center.y, explosion_center.z);

    // Watch out, this has to be removed for chain reaction, currently no chain reaction so leave it in, change this in case of chain reaction being added back in
    if (!curr_vox_material) return;

    // check if player is within explosion
    if (glm::length(tmt::engine.ecs.get_component<tmt::Transform>(player_entity).get_world_position() - explosion_world_pos) <= thermite_ore_settings.radius_explosion) {
        tmt::engine.ecs.get_component<Player>(player_entity).health.value -= thermite_ore_settings.damage_explosion;
    }
    // remove initial voxel (explosion center)
    // vox_renderer->resource->blas->remove_voxel(cx, cy, cz);
    // vox_renderer->resource->set_dirty();

    // spawn an emitter if the center actually still exists
    spawn_emitter(explosion_world_pos);

    // delete the initial voxel
    tmt::engine.ecs.systems.get<tmt::Destruction>().destroy_voxel(voxel_entity, explosion_center);

    // refactor: use sphere check instead
    auto sphere_check_result = tmt::engine.ecs.systems.get<tmt::Physics>().overlap_sphere(explosion_world_pos, thermite_ore_settings.radius_explosion, thermite_ore_settings.layer_mask);
    for (auto& result_pair : sphere_check_result) {
        auto result_entity_world_pos = tmt::engine.ecs.get_component<tmt::Transform>(result_pair.first).get_world_position();
        auto result_entity_local_pos = tmt::engine.ecs.get_component<tmt::Transform>(result_pair.first).get_local_position();

        // voxel list to accumulate all voxels that need to be destroyed
        std::vector<glm::uvec3> voxel_list;

        // Guard for voxel renderer
        if (auto result_vox_renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(result_pair.first)) {
            // Loop over resulting voxels
            for (auto& result_voxel : result_pair.second) {
                auto* curr_vox_material = result_vox_renderer->resource->blas->get_voxel(result_voxel.second.x, result_voxel.second.y, result_voxel.second.z);
                // Skip voxels without a material
                if (!curr_vox_material) {
                    continue;
                }
                auto curr_vox_type = curr_vox_material->type;
                if (curr_vox_type == tmt::Material::Type::THERMITE) {
                    // Add to " new thermite to explode"
                    // new_thermite_to_explode.insert(std::make_pair(result_pair.first, result_voxel.second));
                }
                // Guard for ore properties
                if (ore_properties_entity != entt::null) {
                    // Check for toughness
                    auto curr_vox_toughness = ore_database.at(curr_vox_type).toughness;
                    if (curr_vox_toughness <= thermite_ore_settings.explosion_strength) {
                        // tmt::engine.ecs.systems.get<tmt::Destruction>().destroy_voxel(result_pair.first, result_voxel.second);
                        voxel_list.push_back(result_voxel.second);
                    }
                } else {
                    tmt::Log::warn("No ore properties found, will explode and remove all voxels on thermite explosion");
                    // tmt::engine.ecs.systems.get<tmt::Destruction>().destroy_voxel(result_pair.first, result_voxel.second);
                    voxel_list.push_back(result_voxel.second);
                }
            }
        }
        tmt::engine.ecs.systems.get<tmt::Destruction>().destroy_voxels(result_pair.first, voxel_list);
    }

    sphere_check_result = tmt::engine.ecs.systems.get<tmt::Physics>().overlap_sphere(explosion_world_pos, thermite_ore_settings.knockback_range, thermite_ore_settings.layer_mask);
    for (auto& result_pair : sphere_check_result) {
        auto result_entity_world_pos = tmt::engine.ecs.get_component<tmt::Transform>(result_pair.first).get_world_position();
        //auto result_entity_local_pos = tmt::engine.ecs.get_component<tmt::Transform>(result_pair.first).get_local_position();

        // apply knockback to voxel entity if it has a voxel body
        if (auto* result_voxel_body = tmt::engine.ecs.try_get_component<tmt::VoxelBody>(result_pair.first)) {
            //if (result_voxel_body->type == tmt::VoxelBody::STATIC) continue;  // skip static bodies, they should not be affected by knockback
            //tmt::engine.polyline.use_color(0.0f, 0.0f, 1.0f, 1.0f);
            //tmt::engine.polyline.draw_line(explosion_world_pos, result_entity_world_pos, 10.0f);
            glm::vec3 diff = (result_entity_world_pos) - explosion_world_pos;
            float falloff = std::clamp(1.0f - (glm::length(diff) / thermite_ore_settings.knockback_range), 0.1f, 1.0f);
            //tmt::Log::info("Applying knockback to entity {}, diff: x:{} y:{} z:{}, falloff: {}", result_pair.first, diff.x, diff.y, diff.z, falloff);
            result_voxel_body->velocity = glm::normalize(diff) * thermite_ore_settings.knockback_strength * falloff;
            // Add a tiny bit of random torque
            result_voxel_body->angular_velocity = glm::vec3(Random::rand_range(-1.0f, 1.0f), Random::rand_range(-1.0f, 1.0f), Random::rand_range(-1.0f, 1.0f));
        }
    }
}

void OreManager::spawn_emitter(glm::vec3 spawn_pos) {
    auto explosion_prefab = vfx_settings.explosion_vfx_prefab;
    float lifetime = vfx_lifetime_thermite_explosion;

    if (lifetime <= 0.0f) {
        tmt::Log::error("lifetime of thermite explosion has not been set up correctly, cannot spawn particle emitter");
        return;
    }

    tmt::Entity instantiated_entity = tmt::PrefabHelper::instantiate_prefab(explosion_prefab.file_location);
    if (instantiated_entity == entt::null) return;
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated_entity);

    transform.set_world_position(spawn_pos);

    tmt::ParticleEmitter* emitter_component = tmt::engine.ecs.try_get_component<tmt::ParticleEmitter>(instantiated_entity);
    if (!emitter_component) {
        tmt::Log::error("Cant spawn particle on emitter, check prefab on ore manager component on entity: {}", entity);
        return;
    }

    emitter_component->active = false;
    emitter_component->should_burst = true;

    emitter_lifetime_table.emplace(instantiated_entity, lifetime);
}

}  // namespace game