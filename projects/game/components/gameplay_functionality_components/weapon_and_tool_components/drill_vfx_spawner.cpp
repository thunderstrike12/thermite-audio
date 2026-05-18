#include "drill_vfx_spawner.hpp"
#include "engine/core/components/emitter.hpp"

void game::DrillVFXSpawner::start() {
    tmt::engine.ecs.get_dispatcher().sink<MineVoxelEvent>().connect<&DrillVFXSpawner::on_shoot>(this);
    tmt::engine.ecs.get_dispatcher().sink<MineNothingEvent>().connect<&DrillVFXSpawner::on_stop_shoot>(this);
    auto* emitter { tmt::engine.ecs.try_get_component<tmt::ParticleEmitter>(emitter_entity) };
    if (emitter == nullptr) {
        tmt::Log::warn("No particle emitter entity component found");
    }
}
void game::DrillVFXSpawner::update(const tmt::FrameData& time) {
    if (is_active) {
        auto elapsed_game_time = tmt::engine.frame_data().elapsed_time;
        if (elapsed_game_time - last_shot_time > 1.f / active_rate_per_second) {
            last_shot_time = elapsed_game_time;
            activate_emitter(true);
        }
    }
}
void game::DrillVFXSpawner::end() {
    tmt::engine.ecs.get_dispatcher().sink<MineVoxelEvent>().disconnect<&DrillVFXSpawner::on_shoot>(this);
    tmt::engine.ecs.get_dispatcher().sink<MineNothingEvent>().disconnect<&DrillVFXSpawner::on_stop_shoot>(this);
}
bool game::DrillVFXSpawner::activate_emitter(const bool active) const {
    auto* emitter { tmt::engine.ecs.try_get_component<tmt::ParticleEmitter>(emitter_entity) };
    if (emitter == nullptr) {
        return false;
    }
    emitter->should_burst = active;
    return true;
}
void game::DrillVFXSpawner::on_shoot(const MineVoxelEvent& e) {
    if (e.weapon_entity != entity) {
        return;
    }
    // trigger vfx
    is_active = true;
}
void game::DrillVFXSpawner::on_stop_shoot(const MineNothingEvent& e) {
    if (e.weapon_entity != entity) {
        return;
    }
    is_active = false;
}