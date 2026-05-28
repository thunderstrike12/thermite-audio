#include "weapon.hpp"

#include "engine/tools/prefab_helper.hpp"
#include "engine/tools/fmt/glm.hpp"

namespace game {

void game::Weapon::start() {}
void game::Weapon::update(const tmt::FrameData& time) {
    // vfx handling
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
}
void game::Weapon::end() {}
bool game::Weapon::update_fire_rate(FireRate& fire_rate) {
    auto elapsed_game_time = tmt::engine.frame_data().elapsed_time;
    if (elapsed_game_time - fire_rate.last_shot_time < 1.f / fire_rate.shots_per_second) {
        return false;
    }
    fire_rate.last_shot_time = elapsed_game_time;
    return true;
}
void game::Weapon::on_shoot(const ShootEvent& e) {
    tmt::Entity weapon_manager_entity = tmt::engine.ecs.view<WeaponManager>().front().entity;
    if (WeaponManager* weapon_manager_component = tmt::engine.ecs.try_get_component<WeaponManager>(weapon_manager_entity)) {
        if (weapon_manager_component->switching) {
            return;
        }
    }
    if (e.shooting_entity != shooting_entity) return;

    // TODO change if we have a third shot
    if (e.secondary_shot) {
        if (!update_fire_rate(secondary_fire_rate)) {
            return;
        }
    } else if (!update_fire_rate(primary_fire_rate)) {
        return;
    }
    if (spawn_location_entity == entt::null) {
        spawn_location_entity = entity;
    }
    const auto& tf = tmt::engine.ecs.get_component<tmt::Transform>(spawn_location_entity);
    tmt::engine.ecs.get_dispatcher().trigger(
        WeaponFiredEvent { .weapon_entity = entity, .secondary_shot = e.secondary_shot, .origin = tf.get_world_position(), .up = tf.get_up(), .direction = tf.get_forward() }
    );
    spawn_emitter();
}

void Weapon::spawn_emitter() {
    if (vfx_spawn_location_entity == entt::null) {
        tmt::Log::info("No spawn location set for weapon component, assume no shoot vfx is supposed to spawn. Entity: {}", entity);
        return;
    }

    if (!weapon_shoot_vfx.resource) {
        tmt::Log::error("Cannot set vfx for shooting, abort emitter spawning. Entity: {}", entity);
        return;
    }

    tmt::Entity instantiated_entity = tmt::PrefabHelper::instantiate_prefab(weapon_shoot_vfx->file_location);
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated_entity);

    transform.set_world_position(tmt::engine.ecs.try_get_component<tmt::Transform>(vfx_spawn_location_entity)->get_world_position());

    tmt::ParticleEmitter* emitter_component = tmt::engine.ecs.try_get_component<tmt::ParticleEmitter>(instantiated_entity);
    if (!emitter_component) {
        tmt::Log::error("Cant spawn particle on emitter, check prefab on ore collector component, on entity: {}", entity);
        return;
    }

    emitter_component->active = false;
    emitter_component->should_burst = true;

    emitter_lifetime_table.emplace(instantiated_entity, emitter_lifetime);
}

}  // namespace game
