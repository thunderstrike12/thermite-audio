#include "projectile_spawner.hpp"

#include "rifle_projectile.hpp"
#include "spawner.hpp"
#include "weapon.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/data_headers/save_entries.hpp"
#include "../player.hpp"

namespace game {

std::string_view ProjectileSpawner::get_name() {
    return "ProjectileSpawner";
}
void ProjectileSpawner::start() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&ProjectileSpawner::on_shoot>(this);
    // TODO load saved data if there is any
    if (auto* weapon { tmt::engine.ecs.try_get_component<Weapon>(entity) }) {
        weapon->primary_fire_rate.shots_per_second = tmt::engine.player_data.get<float>(RIFLE_FIRE_DATA, weapon->primary_fire_rate.shots_per_second);
    }

    if (!tmt::engine.ecs.valid(player_entity)) {
        player_entity = tmt::engine.ecs.view<Player>(entt::exclude_t {}).front().entity;  // Assuming there's only one player entity in the game
    }
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
    // TODO load saved data if there is any
    rifle_projectile.explosion_parameters = tmt::engine.player_data.get<ExplosionParameters>(RIFLE_PROJECTILE_DATA, rifle_projectile.explosion_parameters);

    auto& bullet_transform = tmt::engine.ecs.get_component<tmt::Transform>(bullet_entity);

    bullet_transform.set_world_position(e.origin);
    bullet_transform.look_at(e.origin + e.direction, e.up);
    rifle_projectile.set_direction(e.direction);

    auto* player = tmt::engine.ecs.try_get_component<Player>(player_entity);
    if (player) {
        player->add_recoil();
    }
}

}  // namespace game
