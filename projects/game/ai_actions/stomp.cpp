#include "stomp.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
// todo: make projects not have to use realtive paths
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "../components/gameplay_functionality_components/player.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/polyline.hpp"

#include <cstdlib>

void Stomp::clean_up(tmt::Entity enemy_entity) const {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    tmt::engine.ecs.get_component<tmt::RigController>(enemy.rig_controller).set_parameter_bool("stomp", false);
    Tweening::tween<float>()  //
        .from(0.0f)
        .to(1.0f)
        .duration(0.2f)
        .ease(Tweening::Ease::IN_OUT_QUAD)
        .on_update([enemy_entity](float alpha, const float* value) {
            //
            auto& enemy_comp = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
            tmt::engine.ecs.get_component<tmt::ConstrainedRig>(enemy_comp.rig_controller).blend = *value;
        });
}

void Stomp::on_start(tmt::Entity enemy_entity) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    tmt::engine.ecs.get_component<tmt::RigController>(enemy.rig_controller).set_parameter_bool("stomp", true);

    Tweening::tween<float>()  //
        .from(1.0f)
        .to(0.0f)
        .duration(0.2f)
        .ease(Tweening::Ease::IN_OUT_QUAD)
        .on_update([enemy_entity](float alpha, const float* value) {
            //
            auto& enemy_comp = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
            tmt::engine.ecs.get_component<tmt::ConstrainedRig>(enemy_comp.rig_controller).blend = *value;
        });
    time = 0.0f;

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    tmt::engine.polyline.use_color(0, 0, 1);
    tmt::engine.polyline.use_line_width(20.0f);
    tmt::engine.polyline.draw_sphere(enemy_entity_pos, 1.0f, 128, 0.5f);
}

void Stomp::on_tick(tmt::Entity enemy_entity, float dt) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    tmt::engine.ecs.get_component<tmt::RigController>(enemy.rig_controller).set_parameter_bool("stomp", true);
    float blend = tmt::engine.ecs.get_component<tmt::ConstrainedRig>(enemy.rig_controller).blend;
    time += dt;
    if (time > enemy.stomp_windup) {
        clean_up(enemy_entity);
        enemy.stomp_timer = 0.0f;
        tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
        const auto& enemy_entity_pos = enemy_transform.get_world_position();

        if (enemy.audio_emitter != nullptr && !enemy.shield_slam_instance.is_valid()) {
            enemy.shield_slam_instance = enemy.audio_emitter->play(enemy.sounds.sound_shield_slam);
            enemy.shield_slam_instance.set_maximum_distance(enemy.aggro_range * 1.25f);  // Multiply be 1.25f to ensure the player can hear it even when at the edge of the range
        }

        // if player in range, deal damage
        auto& player_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy.player);
        const auto& player_pos = player_transform.get_world_position();
        if (glm::length(player_pos - enemy_entity_pos) <= enemy.stomp_radius) {
            tmt::engine.ecs.get_component<game::Player>(enemy.player).health.value -= enemy.stomp_damage;
        }

        tmt::engine.polyline.use_color(0, 1, 0);
        tmt::engine.polyline.use_line_width(20.0f);
        tmt::engine.polyline.draw_sphere(enemy_entity_pos, 1.0f, 128, 0.5f);
    }
}

bool Stomp::is_done(tmt::Entity enemy_entity) const {
    return false;
}

void Stomp::on_interrupt(tmt::Entity enemy_entity) {
    clean_up(enemy_entity);
}
