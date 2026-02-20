#include "projectile_spawner.hpp"

#include "rifle_projectile.hpp"
#include "spawner.hpp"
#include "weapon.hpp"

namespace game {

std::string_view ProjectileSpawner::get_name() {
    return "ProjectileSpawner";
}
void ProjectileSpawner::start() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&ProjectileSpawner::on_shoot>(this);
}
void ProjectileSpawner::update(const tmt::FrameData& time) {}
void ProjectileSpawner::end() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&ProjectileSpawner::on_shoot>(this);
}
void ProjectileSpawner::on_shoot(const WeaponFiredEvent& e) const {
    // only trigger if we are on the same entity as the weapon fired
    if (e.weapon_entity != entity) {
        return;
    }
    auto& spawner = tmt::engine.ecs.get_component<Spawner>(entity);

    auto bullet_entity = spawner.spawn();
    if (tmt::engine.ecs.valid(bullet_entity) == false) return;

    auto& rifle_projectile = tmt::engine.ecs.get_component<RifleProjectile>(bullet_entity);

    auto& bullet_transform = tmt::engine.ecs.get_component<tmt::Transform>(bullet_entity);

    bullet_transform.set_world_position(e.origin);
    bullet_transform.look_at(e.origin + e.direction, e.up);
    rifle_projectile.set_direction(e.direction);
}

}  // namespace game
