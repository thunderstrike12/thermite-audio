#include "explosion.hpp"

#include "engine/core/polyline.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "projects/game/components/development_tools/debug_line_helper.hpp"
#include "projects/game/components/managers/ore_manager.hpp"
#include "projects/game/data_headers/ore_properties.hpp"

void game::Explosion::explode() const {
    // for thermite explosions
    OreManager* ore_manager { nullptr };
    {
        auto ore_manager_view = tmt::engine.ecs.view<OreManager>();
        if (!ore_manager_view.empty()) {
            ore_manager = tmt::engine.ecs.try_get_component<OreManager>(ore_manager_view.front().entity);
        } else {
            tmt::Log::warn("No ore manager found in scene, add one if you want to use custom ore behaviour");
            return;
        }
    }
    OreProperties* ore_properties { nullptr };

    {
        auto ore_prop_view = tmt::engine.ecs.view<OreProperties>();
        if (!ore_prop_view.empty()) {
            ore_properties = tmt::engine.ecs.try_get_component<OreProperties>(ore_prop_view.front().entity);
        } else {
            tmt::Log::warn("No ore properties found in scene, add one if you want to use custom ore behaviour");
            return;
        }
    }

    auto& ore_database = ore_properties->ores;

    // iterate over all hits and check their toughness
    const auto hits = tmt::engine.ecs.systems.get<tmt::Physics>().overlap_sphere(get_position(), param.radius, param.mask);
    for (const auto& [voxel_entity, voxels] : hits) {
        auto* resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(voxel_entity).resource.resource.get();

        for (const auto& [sqr_distance, voxel_coord] : voxels) {
            auto* voxel_material = resource->blas->get_voxel(voxel_coord.x, voxel_coord.y, voxel_coord.z);
            const auto ore_type = voxel_material->type;
            const auto ore_toughness = ore_database.at(ore_type).toughness;

            switch (ore_type) {
                case tmt::Material::Type::THERMITE:

                    ore_manager->initiate_thermite_explosion(voxel_entity, voxel_coord);

                    break;
                case tmt::Material::Type::NONE:
                case tmt::Material::Type::COPPER:
                case tmt::Material::Type::TITANIUM:
                case tmt::Material::Type::STEEL:
                case tmt::Material::Type::REINFORCED_STEEL: {
                    auto radius = param.radius;
                    float t = sqr_distance / (radius * radius);
                    auto power = (1.0f - param.distance_strength_curve.eval(t)) * param.explosion_power;
                    if (ore_toughness < power) {
                        resource->blas->remove_voxel(voxel_coord.x, voxel_coord.y, voxel_coord.z);
                    }
                } break;
            }
        }
        resource->set_dirty();
    }

    tmt::engine.ecs.destroy_entity(entity);
}
void game::Explosion::start() {
    explode();
}
void game::Explosion::draw_debug_lines() const {
    DebugLineConfig cfg {};

    cfg.set_values();

    tmt::engine.polyline.draw_sphere(get_position(), param.radius);
}
glm::vec3 game::Explosion::get_position() const {
    return tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position();
}
