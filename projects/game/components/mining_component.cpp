#include "mining_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/logger.hpp"
#include "weapon.hpp"
#include "engine/core/polyline.hpp"

using namespace game;

void MiningComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&MiningComponent::on_weapon_fired>(this);
}

void MiningComponent::update(const tmt::FrameData& time) {}

void MiningComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&MiningComponent::on_weapon_fired>(this);
}

void MiningComponent::on_weapon_fired(const WeaponFiredEvent& e) {
    if (e.weapon_entity != entity) return;
    mine(e.origin, e.direction);
}

void MiningComponent::mine(glm::vec3 origin, glm::vec3 dir) {
    if (!stencil) {
        tmt::Log::error("Mining component has no valid stencil!");
        return;
    }

    const tmt::Ray ray_cast = tmt::Ray(origin, dir);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray_cast);
    cfg.set_values();
    // TODO maybe add a generic resource for the time
    tmt::engine.polyline.draw_line(origin, origin + hit.distance * dir, 2.0f);

    if (hit.miss() == false) {
        auto* resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(hit.entity).resource.resource.get();
        resource->blas->subtract(stencil.resource.get(), hit.coord);
        resource->set_dirty();
    }
}
