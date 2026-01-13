#include "audio_listener.hpp"

#include "engine.hpp"
#include "engine/core/audio.hpp"
#include "core/logger.hpp"

#include <fmod_studio.hpp>
#include <fmod_errors.h>

namespace tmt {

void AudioListener::set_weight(const float weight) {
    const FMOD_RESULT result = engine.audio.system->setListenerWeight(listener_index, weight);
    if (result != FMOD_OK) {
        Log::error(Log::Scope::ENGINE, "FMOD, Failed to set listener weight: {}", FMOD_ErrorString(result));
        return;
    }

    cached_weight = weight;
}

float AudioListener::get_weight() const {
    const FMOD_RESULT result = engine.audio.system->getListenerWeight(listener_index, &cached_weight);
    if (result != FMOD_OK) Log::error(Log::Scope::ENGINE, "FMOD, Failed to get listener weight: {}", FMOD_ErrorString(result));

    return cached_weight;
}

}  // namespace tmt