#include "fire_missiles.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "../components/gameplay_functionality_components/enemy_components/missile.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <cstdlib>

void FireMissiles::on_start(tmt::Entity enemy_entity) {
    tmt::Log::error("missile onstart");
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    missiles = enemy.missile_burst;
}

void FireMissiles::on_tick(tmt::Entity enemy_entity, float dt) {
    game::MediumEnemy& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    enemy.kite_player();

    interval_timer += dt;
    if (interval_timer > enemy.burst_interval) {
        interval_timer -= enemy.burst_interval;

        auto* audio_emitter = tmt::engine.ecs.try_get_component<tmt::AudioEmitter>(enemy_entity);
        if (audio_emitter != nullptr) {
            const tmt::AudioInstance3D instance = audio_emitter->play(enemy.sounds.sound_missile_fire);
            instance.set_maximum_distance(enemy.aggro_range * 1.25f);  // Multiply be 1.25f to ensure the player can hear it even when at the edge of the range
        }
        auto missile_entity = tmt::engine.ecs.create_entity();
        auto& missile_comp = tmt::engine.ecs.add_component<game::Missile>(missile_entity);
        missile_comp.enemy_entity = enemy_entity;
        missiles--;
    }

    if (missiles == 0) {
        auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
        enemy.missile_timer = 0.0f;
        auto* ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(enemy_entity);
        if (!ws) return;
        ws->set_fact(tmt::FactId("m_missiles_ready"), false);
    }
}

bool FireMissiles::is_done(tmt::Entity /*enemy_entity*/) const {
    return false;
}

void FireMissiles::on_interrupt(tmt::Entity enemy_entity) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    enemy.missile_timer = 0.0f;
    auto* ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(enemy_entity);
    if (!ws) return;
    ws->set_fact(tmt::FactId("m_missiles_ready"), false);
}
