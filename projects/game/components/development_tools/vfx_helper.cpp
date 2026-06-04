#include "vfx_helper.hpp"

namespace game {

void GameVFXHelper::activate_vfx_emitter(tmt::Entity emitter_entity) {
    if (tmt::ParticleEmitter* emitter_component = tmt::engine.ecs.try_get_component<tmt::ParticleEmitter>(emitter_entity)) {
        emitter_component->active = true;
    } else {
        tmt::Log::error("Could not activate emitter on entity {}", emitter_entity);
    }
}

void GameVFXHelper::deactivate_vfx_emitter(tmt::Entity emitter_entity) {
    if (tmt::ParticleEmitter* emitter_component = tmt::engine.ecs.try_get_component<tmt::ParticleEmitter>(emitter_entity)) {
        emitter_component->active = false;
    } else {
        tmt::Log::error("Could not deactivate emitter on entity {}", emitter_entity);
    }
}

void GameVFXHelper::burst_vfx_emitter(tmt::Entity emitter_entity) {
    if (tmt::ParticleEmitter* emitter_component = tmt::engine.ecs.try_get_component<tmt::ParticleEmitter>(emitter_entity)) {
        emitter_component->should_burst = true;
    } else {
        tmt::Log::error("Could not burst emitter on entity {}", emitter_entity);
    }
}

}  // namespace game