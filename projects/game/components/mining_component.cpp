#include "mining_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "game_input.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "weapon.hpp"

using namespace game;

void MiningComponent::start() {
    // Start listening to weapon fired event
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&MiningComponent::mine_on_weapon_fired>(this);
}

void MiningComponent::update(const tmt::FrameData& time) {
    // Nothing for now
}

void MiningComponent::end() {
    // Stop listening to weapon fired event
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&MiningComponent::mine_on_weapon_fired>(this);
}

void MiningComponent::mine(glm::vec3 origin, glm::vec3 dir) {
    if (!stencil) {
        tmt::Log::error("Mining component has no valid stencil!");
        return;
    }
    // Create a ray
    const tmt::Ray ray_cast = tmt::Ray(origin, dir);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray_cast);

    // tmt::Log::info("Ray from x: {} y: {}", ray_origin_x, ray_origin_y); //use this for debugging

    if (hit.miss() == false) {
        auto* resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(hit.entity).resource.resource.get();
        resource->blas->subtract(stencil.resource.get(), hit.coord);
        resource->set_dirty();
    }
}

void MiningComponent::mine_on_weapon_fired(const WeaponFiredEvent& e) const {
    if (!stencil) {
        tmt::Log::error("Mining component has no valid stencil!");
        return;
    }
    // Create a ray from the event
    const tmt::Ray ray_cast = tmt::Ray(e.origin, e.direction);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray_cast);

    // tmt::Log::info("Ray from x: {} y: {}", ray_origin_x, ray_origin_y); //use this for debugging

    if (hit.miss() == false) {
        auto* resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(hit.entity).resource.resource.get();
        resource->blas->subtract(stencil.resource.get(), hit.coord);
        resource->set_dirty();
    }
}
