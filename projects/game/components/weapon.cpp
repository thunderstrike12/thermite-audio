#include "weapon.hpp"

#include "engine/tools/fmt/glm.hpp"

void game::Weapon::start() {}
void game::Weapon::update(const tmt::FrameData& time) {}
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
    // tmt::Log::info("{} entity shot from {} to {}", shooting_entity, tf.get_world_position(), tf.get_forward());
    tmt::engine.ecs.get_dispatcher().trigger(
        WeaponFiredEvent { .weapon_entity = entity, .secondary_shot = e.secondary_shot, .origin = tf.get_world_position(), .up = tf.get_up(), .direction = tf.get_forward() }
    );
}
