#include "audio_system.hpp"

#include "core/logger.hpp"

#include "fourier.hpp"
#include "engine.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "audio_system.hpp"

namespace tmt {

void tmt::AudioSystem::on_start() {}

void AudioSystem::on_update(const FrameData&) {
    for (const auto&& [entity, rig] : engine.ecs.view<tmt::Fourier>().each()) {

    }
}

void AudioSystem::on_end() {}

}  // namespace tmt