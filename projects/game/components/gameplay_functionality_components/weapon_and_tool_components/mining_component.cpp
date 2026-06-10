#include "mining_component.hpp"

#include "weapon.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/components/managers/ore_manager.hpp"
#include "engine/systems/physics/destruction_system.hpp"
#include "projects/game/data_headers/save_entries.hpp"

#include "engine/steam/achievements.hpp"
#include "engine/steam/steam_api.hpp"

#include <engine/tools/fmt/glm.hpp>
#include <glm/detail/_noise.hpp>

#include "../player.hpp"

#include <cmath>

using namespace game;

std::vector<glm::vec3> MiningComponent::compute_ray_origins(const tmt::Transform& transform) const {
    auto ray_origin = transform.get_world_position();

    std::vector<glm::vec3> origins;
    origins.reserve(ray_cylinder.ray_amount);
    glm::vec3 up = transform.get_up();
    glm::vec3 right = transform.get_right();
    constexpr float golden_angle = 2.399963229728f;
    // TODO copied code from debug draw, might be able to simplify it
    // TODO can also be copied in a buffer at the start and only add to the new origin if sin and cos prove to be too  intensive
    for (size_t i = 0; i < ray_cylinder.ray_amount; i++) {
        float elapsed_time_index = static_cast<float>(i) + glm::mod(tmt::engine.frame_data().elapsed_time * ray_cylinder.rotating_speed, 1.0f);
        const float a0 = golden_angle * elapsed_time_index;
        const float radius = std::sqrt(elapsed_time_index / static_cast<float>(ray_cylinder.ray_amount)) * ray_cylinder.base_radius;

        origins.emplace_back(ray_origin + radius * (right * glm::cos(a0) + up * glm::sin(a0)));
    }
    return origins;
}
void MiningComponent::assign_database() {
    auto view = tmt::engine.ecs.view<OreProperties>();
    for (auto& viewElement : view) {
        auto& ore_properties = tmt::engine.ecs.get_component<OreProperties>(viewElement.entity);
        ore_database = &ore_properties.ores;
    }
}
void MiningComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&MiningComponent::on_weapon_fired>(this);
    tmt::engine.ecs.get_dispatcher().sink<ReleaseShootEvent>().connect<&MiningComponent::on_stop_mining>(this);

    // get a pointer to the database
    assign_database();

    // Set ore manager
    auto ore_manager_view = tmt::engine.ecs.view<OreManager>();
    if (!ore_manager_view.empty()) {
        ore_manager = tmt::engine.ecs.try_get_component<OreManager>(ore_manager_view.front().entity);
    } else {
        tmt::Log::warn("No ore manager found in scene, add one if you want to use custom ore behaviour");
    }

    // initialize the save data with a decent default, otherwise, set the data from the mining data
    if (auto* weapon { tmt::engine.ecs.try_get_component<Weapon>(entity) }) {
        auto mining_data { tmt::engine.player_data.get<MiningData>(
            MINING_DATA, { .base_radius = ray_cylinder.base_radius,
                           .ray_distance = ray_cylinder.ray_distance,
                           .ray_amount = ray_cylinder.ray_amount,
                           .rays_per_second = weapon->primary_fire_rate.shots_per_second }
        ) };

        ray_cylinder.ray_distance = mining_data.ray_distance;
        ray_cylinder.base_radius = mining_data.base_radius;
        ray_cylinder.ray_amount = mining_data.ray_amount;

        weapon->primary_fire_rate.shots_per_second = mining_data.rays_per_second;
    }
}

void MiningComponent::update(const tmt::FrameData& time) {
    has_drawn_debug = true;
}

tmt::Transform* MiningComponent::get_transform() const {
    tmt::Transform* transform;
    if (ray_cylinder.base_transform_entity == entt::null) {
        transform = &tmt::engine.ecs.get_component<tmt::Transform>(entity);
    } else {
        transform = &tmt::engine.ecs.get_component<tmt::Transform>(ray_cylinder.base_transform_entity);
    }
    return transform;
}

void MiningComponent::draw_debug_lines() const {
    cfg.set_values();
    auto& polyline = tmt::engine.polyline;

    tmt::Transform* transform = get_transform();
    auto ray_origin = transform->get_world_position();

    glm::vec3 forward = transform->get_forward();

    polyline.draw_world_circle(ray_origin, forward, ray_cylinder.base_radius);
    polyline.draw_world_circle(ray_origin + ray_cylinder.ray_distance * forward, forward, ray_cylinder.base_radius);
    if (has_drawn_debug == false) {
        for (const glm::vec3& origin : previous_computed_origins) {
            polyline.draw_line(origin, origin + ray_cylinder.ray_distance * forward, time_draw_rays_in_debug);
        }
    }
}

void MiningComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&MiningComponent::on_weapon_fired>(this);
    tmt::engine.ecs.get_dispatcher().sink<ReleaseShootEvent>().disconnect<&MiningComponent::on_stop_mining>(this);
}

void MiningComponent::on_weapon_fired(const WeaponFiredEvent& e) {
    if (e.weapon_entity != entity) return;

    auto* player = tmt::engine.ecs.try_get_component<Player>(player_entity);
    if (player) {
        player->add_camera_shake(player->camera_shake_settings.get_drill_intensity());
    }

    if (mining_activate_sound.is_valid() && !mining_activate_sound_instance.is_valid()) mining_activate_sound_instance = mining_activate_sound.play();

    mine(e.direction);
}
void MiningComponent::on_stop_mining(const ReleaseShootEvent& e) {
    // TODO this will trigger no matter what the entity is for now
    stopped_mining = true;
    tmt::engine.ecs.get_dispatcher().trigger<MineNothingEvent>({ entity });

    if (mining_sound_instance.is_valid()) {
        mining_sound_instance.stop();
        mining_sound_instance = {};
    }

    tmt::Log::info("Stopped mining, reset previous hits");
}

void MiningComponent::handle_voxel(const VoxelID& voxel_id) {
    // If the entity is not valid anymore skip it
    if (!tmt::engine.ecs.valid(voxel_id.entity_id)) return;

    auto* resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(voxel_id.entity_id).resource.resource.get();
    const auto voxel_coord = voxel_id.unpack_coord();
    tmt::Material* material = resource->blas->get_voxel(voxel_coord.x, voxel_coord.y, voxel_coord.z);

    // TODO I have no idea why this triggers
    if (material == nullptr) {
        tmt::Log::error("Somehow the voxel that was supposed to be destroyed at {}, returns a nullptr", voxel_coord);
        return;
    }
    auto ore_type = material->type;
    auto ore_toughness = ore_database->at(ore_type).toughness;

    switch (ore_type) {
        case tmt::Material::Type::THERMITE:
            if (ore_toughness < tmt::engine.frame_data().elapsed_time - mining_voxels.at(voxel_id)) {
                if (ore_manager) {
                    ore_manager->initiate_thermite_explosion(voxel_id.entity_id, voxel_coord);
                } else {
                    tmt::Log::error("Tried to mine thermite ore with no ore manager in scene.");
                }
            }
            break;
        case tmt::Material::Type::NONE:
        case tmt::Material::Type::COPPER:
        case tmt::Material::Type::TITANIUM:
        case tmt::Material::Type::REINFORCED_STEEL:
        case tmt::Material::Type::STEEL:
            if (ore_toughness < tmt::engine.frame_data().elapsed_time - mining_voxels.at(voxel_id)) {
                // resource->blas->remove_voxel(voxel_coord.x, voxel_coord.y, voxel_coord.z);
                // resource->set_dirty();
                auto entities { tmt::engine.ecs.systems.get<tmt::Destruction>().destroy_voxel(voxel_id.entity_id, voxel_coord) };
                if (entities.size() > 1) {
                    tmt::Log::info("Mining has separated objects");
                }

                tmt::engine.steam.achievement->stats.voxels_mined_count++;
                tmt::engine.steam.achievement->store_stats = true;
            }
            break;
    }
    tmt::engine.ecs.get_dispatcher().trigger<MineVoxelEvent>({ entity });
    has_drawn_debug = false;
}
void MiningComponent::mine(const glm::vec3& dir) {
    // TODO reset all state about voxels that are mined
    if (stopped_mining == true) {
        mining_voxels.clear();
        stopped_mining = false;
    }

    previous_computed_origins = compute_ray_origins(*get_transform());
    // Get a list of all unique voxels that we hit this frame
    UniqueVoxelsSet new_mining_voxel_set {};
    for (const auto& origin : previous_computed_origins) {
        const tmt::Ray ray_cast = tmt::Ray(origin, dir);
        const tmt::Hit hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray_cast, ray_mask);

        if (hit.miss() == false && hit.distance < ray_cylinder.ray_distance) {
            if (!mining_sound_instance.is_valid()) {
                mining_sound_instance = mining_sound.play();
            }
            new_mining_voxel_set.insert(VoxelID { hit.entity, hit.coord });
            // handle_ore(hit);
        }
    }
    if (new_mining_voxel_set.empty() == true) {
        tmt::engine.ecs.get_dispatcher().trigger<MineNothingEvent>({ entity });

        if (mining_sound_instance.is_valid()) {
            mining_sound_instance.stop();
            mining_sound_instance = {};
        }
    }
    // Comparing with the previous hit we get 3 options
    // we do not have a voxel that has previously been in the dictionary, but not in this frame of mining, meaning we have to remove it

    std::erase_if(mining_voxels, [&](const auto& pair) { return new_mining_voxel_set.contains(pair.first) == false; });
    auto current_time = tmt::engine.frame_data().elapsed_time;
    for (const VoxelID& voxel_id : new_mining_voxel_set) {
        // we do not find it in the dictionary, we add it to the dictionary with a new timestamp

        auto [it, inserted] = mining_voxels.try_emplace(voxel_id, current_time);
        // we find it in the dictionary, this means we need to check for time, and destroy it if needed
        if (inserted == false) {
            handle_voxel(voxel_id);
        }
    }
}
