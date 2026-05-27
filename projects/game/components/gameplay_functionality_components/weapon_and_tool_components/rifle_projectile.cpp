#include "rifle_projectile.hpp"

#include "spawner.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/shared/ray.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "glm/detail/_noise.hpp"


namespace game {

void RifleProjectile::start() {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    previous_position = transform.get_world_position();
}
void RifleProjectile::update(const tmt::FrameData& time) {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    auto delta = direction * time.delta_time * movement_speed;

    const float step_distance = glm::length(delta);
    auto dir { glm::normalize(delta) };
    const tmt::Ray ray_cast = tmt::Ray(previous_position, dir);
    const tmt::Hit hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray_cast, layer_mask);
    if (hit.distance < step_distance) {
        auto explode_position { previous_position + dir * hit.distance };

        collide(hit, explode_position);
        return;
    }

    transform.translate(delta);
    last_step_length = step_distance;
    previous_position = transform.get_world_position();
}
void RifleProjectile::end() {}
void RifleProjectile::draw_debug_lines() const {
    tmt::engine.polyline.use_line_width(2.5f);
    tmt::engine.polyline.draw_arrow(previous_position, direction, last_step_length);
}
void RifleProjectile::spawn_explosion(const glm::vec3& explode_position) const {
    auto* spawner = tmt::engine.ecs.try_get_component<Spawner>(entity);
    if (spawner == nullptr) {
        tmt::Log::warn("No spawner found on entity {}", entity);
        return;
    }
    // we do initialize by injection
    auto explosion_entity = spawner->spawn();
    auto* explosion { tmt::engine.ecs.try_get_component<Explosion>(explosion_entity) };
    if (explosion == nullptr) {
        tmt::Log::warn("No explosion component found on entity {}", explosion_entity);
        return;
    }

    tmt::engine.ecs.get_component<tmt::Transform>(explosion_entity).set_world_position(explode_position);
    explosion->param = explosion_parameters;
}
void RifleProjectile::collide(const tmt::Hit& hit, const glm::vec3& explode_position) const {
    auto collision_entity = hit.entity;

    // spawn vfx sounds
    tmt::engine.ecs.get_dispatcher().trigger(ProjectileHitEvent { .projectile_entity = entity, .hit_entity = collision_entity });

    // only destroy voxels if the hit entity isn't on a protected layer (e.g. barge)
    auto& body = tmt::engine.ecs.get_component<tmt::VoxelBody>(hit.entity);
    if (protected_mask.test(body.layer) == false) {
        spawn_explosion(explode_position);
    }

    tmt::engine.ecs.destroy_entity(entity);
}

}  // namespace game
