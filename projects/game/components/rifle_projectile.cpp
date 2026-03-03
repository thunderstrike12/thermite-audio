#include "rifle_projectile.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/shared/ray.hpp"
#include "engine/tools/fmt/glm.hpp"

namespace game {

void RifleProjectile::start() {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    previous_position = transform.get_world_position();
}
void RifleProjectile::update(const tmt::FrameData& time) {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    auto delta = direction * time.delta_time * movement_speed;

    const float step_distance = glm::length(delta);
    const tmt::Ray ray_cast = tmt::Ray(previous_position, glm::normalize(delta));
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray_cast);

    // TODO this is very dumb code that should be updated when we have proper raycasts check
    if (hit.entity != entt::null && hit.distance < step_distance && hit.entity != entity) {
        // tmt::Log::info("Would collide with entity {} at distance {}, step {}", hit.entity, hit.distance, step_distance);

        collide(hit);
        return;
    }

    transform.translate(delta);
    last_step_length = step_distance;
    previous_position = transform.get_world_position();
}
void RifleProjectile::end() {}
void RifleProjectile::draw_debug_lines() const {
    // tmt::engine.polyline.use_color(tmt::colors::RED);
    tmt::engine.polyline.use_line_width(2.5f);
    tmt::engine.polyline.draw_arrow(previous_position, direction, last_step_length);
}
void RifleProjectile::collide(const tmt::Hit& hit) const {
    auto collision_entity = hit.entity;

    // spawn vfx sounds
    tmt::engine.ecs.get_dispatcher().trigger(ProjectileHitEvent { .projectile_entity = entity, .hit_entity = collision_entity });

    // substract voxels
    // TODO use something else than a voxel renderer to do stuff

    auto* resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(hit.entity).resource.resource.get();
    resource->blas->subtract(stencil.resource.get(), hit.coord);
    resource->set_dirty();
    tmt::engine.ecs.destroy_entity(entity);
}

}  // namespace game
