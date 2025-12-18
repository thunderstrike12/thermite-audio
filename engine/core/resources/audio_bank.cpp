#include "audio_bank.hpp"

#include "engine.hpp"
#include "core/logger.hpp"
#include "core/io.hpp"
#include "core/audio.hpp"

#include <fmod_errors.h>
#include <fmod_studio.hpp>

namespace tmt {

bool AudioBank::load() {
    std::vector bank_data = IO::read_file(file_location);
    if (bank_data.empty()) return false;

    bank = engine.audio.init_bank(bank_data);
    if (bank == nullptr) return false;

#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG)
    if (is_master) {
        IO::FileLocation string_bank_location = file_location;
        string_bank_location.relative_path.replace_extension(".strings.bank");

        bank_data = IO::read_file(string_bank_location);
        if (bank_data.empty()) {
            Log::warn(Log::Scope::ENGINE, "Failed to load the Master.strings.bank, audio events/parameters won't have names.");
        } else {
            string_bank = engine.audio.init_bank(bank_data);
        }
    }
#endif

    return true;
}

void AudioBank::unload() {
    if (string_bank != nullptr) string_bank->unload();

    bank->unload();
}

std::string AudioBank::get_path() const {
#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG)
    if (bank == nullptr) Log::error(Log::Scope::ENGINE, "FMOD, Invalid AudioEvent for get_path");

    // This function only returns useful paths in the editor or in debug mode, since these aren't necessary for audio to function, and it allows us to skip loading string banks in release
    // game.
    std::string name;
    name.resize(512);

    int size = 0;
    const FMOD_RESULT result = bank->getPath(name.data(), static_cast<int>(name.size()), &size);
    name.resize(size);

    if (result != FMOD_OK) {
        Log::error(Log::Scope::ENGINE, "FMOD, Failed to get the path of the event: {}", FMOD_ErrorString(result));
        return "";
    }

    return name;
#else
    return "";
#endif
}

std::vector<AudioEvent> AudioBank::get_audio_events() const {
    int description_count;
    FMOD_RESULT result = bank->getEventCount(&description_count);
    if (result != FMOD_OK) {
        Log::error(Log::Scope::ENGINE, "FMOD, Failed to get AudioEvent count from bank resource: {}", FMOD_ErrorString(result));
        return {};
    }

    if (description_count <= 0) return {};

    std::vector<FMOD::Studio::EventDescription*> descriptions(description_count);
    result = bank->getEventList(descriptions.data(), description_count, nullptr);

    if (result != FMOD_OK) {
        Log::error(Log::Scope::ENGINE, "FMOD, Failed to get AudioEvents from bank resource: {}", FMOD_ErrorString(result));
        return {};
    }

    std::vector<AudioEvent> events;
    events.reserve(descriptions.size());
    for (FMOD::Studio::EventDescription* description : descriptions) {
        events.emplace_back(description);
    }
    return events;
}

std::vector<VolumeControl> AudioBank::get_volume_controls() const {
    int vca_count;
    FMOD_RESULT result = bank->getVCACount(&vca_count);
    if (result != FMOD_OK) {
        Log::error(Log::Scope::ENGINE, "FMOD, Failed to get VolumeControl count from bank resource: {}", FMOD_ErrorString(result));
        return {};
    }

    if (vca_count <= 0) return {};

    std::vector<FMOD::Studio::VCA*> vcas(vca_count);
    result = bank->getVCAList(vcas.data(), vca_count, nullptr);

    if (result != FMOD_OK) {
        Log::error(Log::Scope::ENGINE, "FMOD, Failed to get VolumeControls from bank resource: {}", FMOD_ErrorString(result));
        return {};
    }

    std::vector<VolumeControl> volume_controls;
    volume_controls.reserve(vcas.size());
    for (FMOD::Studio::VCA* vca : vcas) {
        volume_controls.emplace_back(vca);
    }
    return volume_controls;
}

}  // namespace tmt