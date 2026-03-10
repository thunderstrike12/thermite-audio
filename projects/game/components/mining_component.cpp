#include "mining_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/tools/fmt/glm.hpp"

using namespace game;

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
}

void MiningComponent::update(const tmt::FrameData& time) {
    has_drawn_debug = true;
}

void MiningComponent::draw_debug_lines() const {
    if (has_drawn_debug) return;
    cfg.set_values();
    auto& polyline = tmt::engine.polyline;
    polyline.draw_line(last_ray.origin, last_ray.origin + last_hit.distance * last_ray.dir, 2.0f);
}

void MiningComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&MiningComponent::on_weapon_fired>(this);
    tmt::engine.ecs.get_dispatcher().sink<ReleaseShootEvent>().disconnect<&MiningComponent::on_stop_mining>(this);
}

void MiningComponent::on_weapon_fired(const WeaponFiredEvent& e) {
    if (e.weapon_entity != entity) return;
    mine(e.origin, e.direction);
}
void MiningComponent::on_stop_mining(const ReleaseShootEvent& e) {
    // TODO this will trigger no matter what the entity is for now
    previous_hit.is_mining = false;
    tmt::Log::info("Stopped mining, reset previous hits");
}

void MiningComponent::handle_ore(const tmt::Hit& hit) {
    auto* resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(hit.entity).resource.resource.get();
    auto ore_type = resource->blas->get_voxel(hit.coord.x, hit.coord.y, hit.coord.z)->type;
    // switched voxel, reset
    if (previous_hit.is_mining == false || hit.coord != previous_hit.voxel_coord) {
        previous_hit.mining_time = tmt::engine.frame_data().elapsed_time;
        previous_hit.voxel_coord = hit.coord;
        previous_hit.type = ore_type;
        previous_hit.is_mining = true;
    }

    switch (ore_type) {
        case tmt::Material::Type::NONE:
            break;
        case tmt::Material::Type::THERMITE:
            break;
        case tmt::Material::Type::COPPER:
        case tmt::Material::Type::TITANIUM:
            auto ore_toughness = ore_database->at(ore_type).toughness;
            if (ore_toughness < tmt::engine.frame_data().elapsed_time - previous_hit.mining_time) {
                resource->blas->remove_voxel(previous_hit.voxel_coord.x, previous_hit.voxel_coord.y, previous_hit.voxel_coord.z);
                resource->set_dirty();
            }
            break;
    }

    has_drawn_debug = false;
}
void MiningComponent::mine(glm::vec3 origin, glm::vec3 dir) {
    if (!stencil) {
        tmt::Log::error("Mining component has no valid stencil!");
        return;
    }

    const tmt::Ray ray_cast = tmt::Ray(origin, dir);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray_cast);
    last_ray = ray_cast;
    last_hit = hit;
    // TODO maybe add a generic resource for the time

    if (hit.miss() == false) {
        handle_ore(hit);
    }
}
