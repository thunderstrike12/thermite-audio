#include "audio.hpp"

#include "engine.hpp"
#include "core/logger.hpp"

#include <fmod_studio.hpp>
#include <fmod_errors.h>

namespace tmt {

namespace {
// Logs the message as an error if the result is not FMOD_OK.
bool TryLogError(const FMOD_RESULT result, const spdlog::string_view_t message) {
    if (result) {
        Log::error(Log::Scope::ENGINE, "FMOD, {}: {}", message, FMOD_ErrorString(result));
        return true;
    }

    return false;
}

// Logs the message as an error if fail is false (used with error check e.g. "pointer == nullptr").
bool TryLogError(const bool fail, const spdlog::string_view_t message) {
    if (fail) Log::error(Log::Scope::ENGINE, "FMOD, {}", message);
    return fail;
}
}  // namespace

bool AudioInstance::is_valid() const { return engine.audio.active_instances.contains(instance); }

void AudioInstance::stop(const FMOD_STUDIO_STOP_MODE stop_mode) const {
    if (TryLogError(!is_valid(), "Invalid AudioInstance for get_path")) return;

    instance->stop(stop_mode);
}

void AudioInstance::set_parameter(const AudioParameter& event_parameter, const float value) const {
    const FMOD_RESULT result = instance->setParameterByID(event_parameter.get_id(), value);

    TryLogError(result, "Failed to set instance parameter");
}

void AudioInstance::set_parameter(const AudioParameter& event_parameter, const int value) const {
    const FMOD_RESULT result = instance->setParameterByID(event_parameter.get_id(), static_cast<float>(value));

    TryLogError(result, "Failed to set instance parameter");
}
void AudioInstance::set_label_parameter(const AudioParameter& event_parameter, const std::string& value) const {
    const FMOD_RESULT result = instance->setParameterByIDWithLabel(event_parameter.get_id(), value.c_str());

    TryLogError(result, "Failed to set parameter with label");
}

bool AudioEvent::is_valid() const { return description != nullptr && description->isValid(); }

std::string AudioEvent::get_path() const {
#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG)
    if (TryLogError(description == nullptr, "Invalid AudioEvent for get_path")) return "";

    // This function only returns useful paths in the editor or in debug mode, since these aren't necessary for audio to function, and it allows us to skip loading string banks in release
    // game.
    std::string name;
    name.resize(512);

    int size = 0;
    const FMOD_RESULT result = description->getPath(name.data(), static_cast<int>(name.size()), &size);
    name.resize(size);

    TryLogError(result, "Failed to get the path of the event");

    return name;
#else
    return "";
#endif
}

std::vector<AudioParameter> AudioEvent::get_parameters() const {
    int parameter_count = 0;
    FMOD_RESULT result = description->getParameterDescriptionCount(&parameter_count);
    if (TryLogError(result, "Failed to get event parameter count")) return {};

    if (parameter_count <= 0) return {};

    std::vector<AudioParameter> parameters;
    parameters.reserve(parameter_count);
    for (int i = 0; i < parameter_count; i++) {
        FMOD_STUDIO_PARAMETER_DESCRIPTION parameter_description;
        result = description->getParameterDescriptionByIndex(i, &parameter_description);
        if (TryLogError(result, "Failed to get event parameter by index")) return {};

        parameters.emplace_back(parameter_description);
    }

    return parameters;
}

AudioInstance AudioEvent::play() const {
    if (TryLogError(description == nullptr, "Invalid AudioEvent for play")) return {nullptr};

    // Play the event, creating an instance.
    FMOD::Studio::EventInstance* event_instance = nullptr;
    FMOD_RESULT result = description->createInstance(&event_instance);
    if (TryLogError(result, "Event instance with this description could not be created")) return {nullptr};

    engine.audio.active_instances.emplace(event_instance);

    result = event_instance->start();
    if (TryLogError(result, "Event instance failed to start")) return {nullptr};

    // Mark it for release immediately, once it ends it can immediately be cleaned up.
    result = event_instance->release();
    if (TryLogError(result, "Event instance failed to mark for release")) return {nullptr};

    return {event_instance};
}

bool VolumeControl::is_valid() const { return vca != nullptr && vca->isValid(); }

std::string VolumeControl::get_path() const {
#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG)
    if (TryLogError(vca == nullptr, "Invalid VolumeControl for get_path")) return "";

    // This function only returns useful paths in the editor or in debug mode, since these aren't necessary for audio to function, and it allows us to skip loading string banks in release
    // game.
    std::string name;
    name.resize(512);

    int size = 0;
    const FMOD_RESULT result = vca->getPath(name.data(), static_cast<int>(name.size()), &size);
    name.resize(size);

    TryLogError(result, "Failed to get the path of the volume control");

    return name;
#else
    return "";
#endif
}

float VolumeControl::get_volume() const {
    if (TryLogError(vca == nullptr, "Invalid VolumeControl for get_volume")) return 0.0f;

    float volume = 0.0f;
    const FMOD_RESULT result = vca->getVolume(&volume);
    if (TryLogError(result, "Failed to get VCA volume")) return 0.0f;

    return volume;
}

void VolumeControl::set_volume(const float volume) const {
    if (TryLogError(vca == nullptr, "Invalid VolumeControl for set_volume")) return;

    const FMOD_RESULT result = vca->setVolume(volume);
    TryLogError(result, "Failed to set VCA volume");
}

void Audio::init() {
    Log::info("Audio Engine: FMOD Studio by Firelight Technologies Pty Ltd.");

    FMOD_RESULT result = FMOD::Studio::System::create(&system);
    if (TryLogError(result, "Failed to create the FMOD Studio System")) return;

    // Initialize FMOD Studio, which will also initialize FMOD Core.
    result = system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_3D_RIGHTHANDED, nullptr);
    if (TryLogError(result, "Failed to initialize the FMOD Studio System")) return;

    result = system->getCoreSystem(&core_system);
    TryLogError(result, "Failed to get the FMOD Studio System after initialization");
}

void Audio::update() {
    system->update();

    std::erase_if(active_instances, [](const FMOD::Studio::EventInstance* instance) {
        FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
        instance->getPlaybackState(&state);
        return state == FMOD_STUDIO_PLAYBACK_STOPPED;
    });
}

void Audio::end() const { system->release(); }

FMOD::Studio::Bank* Audio::init_bank(const std::vector<char>& bank_data) const {
    FMOD::Studio::Bank* bank = nullptr;
    FMOD_RESULT result = system->loadBankMemory(bank_data.data(), static_cast<int>(bank_data.size()), FMOD_STUDIO_LOAD_MEMORY, FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
    if (TryLogError(result, "Bank could not be initialized from memory")) return nullptr;

    // Load all the bank's sample data immediately.
    result = bank->loadSampleData();
    if (TryLogError(result, "Bank could not load sample data")) return nullptr;

    // Enable this to wait for loading to finish.
    result = system->flushSampleLoading();
    if (TryLogError(result, "Bank could not flush sample loading")) return nullptr;

    return bank;
}

AudioEvent Audio::get_event(const FMOD_GUID& guid) const {
    FMOD::Studio::EventDescription* description;
    system->getEventByID(&guid, &description);

    return {description};
}

VolumeControl Audio::get_volume_control(const FMOD_GUID& guid) const {
    FMOD::Studio::VCA* vca;
    const FMOD_RESULT result = system->getVCAByID(&guid, &vca);

    if (TryLogError(result, "Failed to get VolumeControl")) return nullptr;

    return {vca};
}

void Audio::stop_all_audio_instances() {
    for (FMOD::Studio::EventInstance* instance : active_instances) {
        instance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
    }

    active_instances.clear();
}

void Audio::set_global_parameter(const AudioParameter& parameter, const float value) const {
    const FMOD_RESULT result = system->setParameterByID(parameter.get_id(), value);

    TryLogError(result, "Failed to set parameter");
}

void Audio::set_global_parameter(const AudioParameter& parameter, int value) const { set_global_parameter(parameter, static_cast<float>(value)); }

void Audio::set_global_label_parameter(const AudioParameter& parameter, const std::string& value) const {
    const FMOD_RESULT result = system->setParameterByIDWithLabel(parameter.get_id(), value.c_str());

    TryLogError(result, "Failed to set parameter with label");
}

}  // namespace tmt