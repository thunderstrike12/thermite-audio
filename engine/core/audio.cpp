#include "audio.hpp"

#include "engine.hpp"
#include "core/logger.hpp"
#include "components/audio_listener.hpp"

#include <fmod_studio.hpp>
#include <fmod_errors.h>

#include "ecs.hpp"
#include "polyline.hpp"
#include "components/audio_emitter.hpp"
#include "systems/physics/components/voxel_body.hpp"

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

// Logs the message as an error if fail is true (used with error checks e.g. "!vca->is_valid()").
bool TryLogError(const bool fail, const spdlog::string_view_t message) {
    if (fail) Log::error(Log::Scope::ENGINE, "Audio, {}", message);
    return fail;
}

// Vector of audio instances that are/were paused due to pausing the game.
std::vector<AudioInstance> paused_game_audio;

}  // namespace

bool AudioInstance::is_valid() const {
    return engine.audio.active_instances.contains(instance) && instance->isValid();
}

void AudioInstance::stop(const FMOD_STUDIO_STOP_MODE stop_mode) const {
    if (TryLogError(!is_valid(), "Invalid AudioInstance for get_path")) return;

    instance->stop(stop_mode);
}

void AudioInstance::set_paused(const bool pause) const {
    if (TryLogError(!is_valid(), "Invalid AudioInstance for set_paused")) return;

    const FMOD_RESULT result = instance->setPaused(pause);
    TryLogError(result, "Failed to set instance paused");
}

bool AudioInstance::get_paused() const {
    if (TryLogError(!is_valid(), "Invalid AudioInstance for get_paused")) return false;

    bool paused = false;
    const FMOD_RESULT result = instance->getPaused(&paused);
    TryLogError(result, "Failed to get instance paused");

    return paused;
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

void AudioInstance3D::set_minimum_distance(const float min) const {
    const FMOD_RESULT result = instance->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MINIMUM_DISTANCE, min);

    TryLogError(result, "Failed to set minimum distance");
}

void AudioInstance3D::set_maximum_distance(const float max) const {
    const FMOD_RESULT result = instance->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MAXIMUM_DISTANCE, max);

    TryLogError(result, "Failed to set maximum distance");
}

void AudioInstance3D::set_3d_position(const glm::vec3& position) const {
    FMOD_3D_ATTRIBUTES attributes = get_3d_attributes();
    attributes.position = std::bit_cast<FMOD_VECTOR>(position);

    const FMOD_RESULT result = instance->set3DAttributes(&attributes);
    TryLogError(result, "Failed to set 3D position attribute");
}

void AudioInstance3D::set_3d_velocity(const glm::vec3& velocity) const {
    FMOD_3D_ATTRIBUTES attributes = get_3d_attributes();
    attributes.velocity = std::bit_cast<FMOD_VECTOR>(velocity);

    const FMOD_RESULT result = instance->set3DAttributes(&attributes);
    TryLogError(result, "Failed to set 3D velocity attribute");
}

void AudioInstance3D::set_3d_forward(const glm::vec3& forward) const {
    FMOD_3D_ATTRIBUTES attributes = get_3d_attributes();
    attributes.forward = std::bit_cast<FMOD_VECTOR>(forward);

    const FMOD_RESULT result = instance->set3DAttributes(&attributes);
    TryLogError(result, "Failed to set 3D forward attribute");
}

void AudioInstance3D::set_3d_up(const glm::vec3& up) const {
    FMOD_3D_ATTRIBUTES attributes = get_3d_attributes();
    attributes.up = std::bit_cast<FMOD_VECTOR>(up);

    const FMOD_RESULT result = instance->set3DAttributes(&attributes);
    TryLogError(result, "Failed to set 3D up attribute");
}

void AudioInstance3D::auto_set_3d_attributes(const Transform& transform, const VoxelBody* voxel_body) const {
    FMOD_VECTOR velocity {};
    if (voxel_body != nullptr) velocity = std::bit_cast<FMOD_VECTOR>(voxel_body->velocity);  // Set the velocity if the entity also has a voxel_body component.

    const FMOD_3D_ATTRIBUTES attributes {
        .position = std::bit_cast<FMOD_VECTOR>(transform.get_world_position()),
        .velocity = velocity,
        .forward = std::bit_cast<FMOD_VECTOR>(transform.get_forward()),
        .up = std::bit_cast<FMOD_VECTOR>(transform.get_up()),
    };

    const FMOD_RESULT result = instance->set3DAttributes(&attributes);
    TryLogError(result, "Failed to auto set 3D attributes");
}

void AudioInstance3D::set_listener_mask(const unsigned int mask) const {
    const FMOD_RESULT result = instance->setListenerMask(mask);
    TryLogError(result, "Failed to set instance listener mask");
}

unsigned int AudioInstance3D::get_listener_mask() const {
    unsigned int mask = 0xFFFFFFFF;

    const FMOD_RESULT result = instance->getListenerMask(&mask);
    TryLogError(result, "Failed to get instance listener mask");

    return mask;
}

float AudioInstance3D::get_minimum_distance() const {
    float min = 0.0f;
    const FMOD_RESULT result = instance->getProperty(FMOD_STUDIO_EVENT_PROPERTY_MINIMUM_DISTANCE, &min);

    TryLogError(result, "Failed to get minimum distance");

    return min;
}

float AudioInstance3D::get_maximum_distance() const {
    float max = 0.0f;
    const FMOD_RESULT result = instance->getProperty(FMOD_STUDIO_EVENT_PROPERTY_MAXIMUM_DISTANCE, &max);

    TryLogError(result, "Failed to get maximum distance");

    return max;
}

FMOD_3D_ATTRIBUTES AudioInstance3D::get_3d_attributes() const {
    FMOD_3D_ATTRIBUTES attributes;
    const FMOD_RESULT result = instance->get3DAttributes(&attributes);
    if (TryLogError(result, "Failed to get instance 3D attributes")) return {};

    return attributes;
}

bool AudioEvent::is_valid() const {
    if (*this == AudioEvent {}) return false;  // Check if the audio event has an invalid uuid (in which case it, itself is invalid).

    const FMOD::Studio::EventDescription* description = engine.audio.get_event_description(uuid);
    return source_bank && description->isValid();
}

bool AudioEvent::is_3d() const {
    const FMOD::Studio::EventDescription* description = engine.audio.get_event_description(uuid);

    bool is_3d = false;
    const FMOD_RESULT result = description->is3D(&is_3d);
    TryLogError(result, "Failed check if event is 3D");

    return is_3d;
}

float AudioEvent::get_length() const {
    const FMOD::Studio::EventDescription* description = engine.audio.get_event_description(uuid);

    int length = 0;
    const FMOD_RESULT result = description->getLength(&length);
    TryLogError(result, "Failed get event length");

    return static_cast<float>(length) / 1000.0f;  // Length in seconds.
}

std::string AudioEvent::get_path() const {
#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG)
    if (TryLogError(!is_valid(), "Invalid AudioEvent for get_path")) return "";

    const FMOD::Studio::EventDescription* description = engine.audio.get_event_description(uuid);

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
    const FMOD::Studio::EventDescription* description = engine.audio.get_event_description(uuid);

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

        parameters.emplace_back(*this, parameter_description.id);
    }

    return parameters;
}

AudioInstance AudioEvent::play() const {
    if (TryLogError(!is_valid(), "Invalid AudioEvent for play.")) return { nullptr };

    const FMOD::Studio::EventDescription* description = engine.audio.get_event_description(uuid);

    // Play the event, creating an instance.
    FMOD::Studio::EventInstance* event_instance = nullptr;
    FMOD_RESULT result = description->createInstance(&event_instance);
    if (TryLogError(result, "Event instance with this description could not be created")) return { nullptr };

    engine.audio.active_instances.emplace(event_instance);

    result = event_instance->start();
    if (TryLogError(result, "Event instance failed to start")) return { nullptr };

    // Mark it for release immediately, once it ends it can immediately be cleaned up.
    result = event_instance->release();
    if (TryLogError(result, "Event instance failed to mark for release")) return { nullptr };

    return { event_instance };
}

AudioInstance3D AudioEvent::play_3d() const {
    if (TryLogError(!is_valid(), "Invalid AudioEvent for play_3d.")) return { nullptr };

    if (TryLogError(!is_3d(), "AudioEvent isn't 3d, invalid call to play_3d.")) return { nullptr };

    AudioInstance instance = play();
    return { instance.instance };
}

glm::vec2 AudioEvent::get_min_max_distance() const {
    if (TryLogError(!is_valid(), "Invalid VolumeControl for get_min_max")) return {};

    const FMOD::Studio::EventDescription* description = engine.audio.get_event_description(uuid);

    float min = 0;
    float max = 0;
    const FMOD_RESULT result = description->getMinMaxDistance(&min, &max);
    if (TryLogError(result, "Failed to get VCA volume")) return {};

    return { min, max };
}

FMOD_STUDIO_PARAMETER_DESCRIPTION AudioEvent::get_parameter_description(const FMOD_STUDIO_PARAMETER_ID& id) const {
    const FMOD::Studio::EventDescription* description = engine.audio.get_event_description(uuid);

    FMOD_STUDIO_PARAMETER_DESCRIPTION parameter_description {};
    const FMOD_RESULT result = description->getParameterDescriptionByID(id, &parameter_description);
    if (TryLogError(result, "Failed to get event parameter by ID")) return {};

    return parameter_description;
}

bool AudioParameter::is_valid() const {
    if (*this == AudioParameter {}) return false;                       // Check if the audio parameter has an invalid uuid (in which case it, itself is invalid).

    if (!source_event.is_valid()) return false;                         // Check if the source event is still valid.

    return source_event.get_parameter_description(id).name != nullptr;  // Check if the audio parameter description is valid.
}

std::string AudioParameter::get_name() const {
    return source_event.get_parameter_description(id).name;
}

float AudioParameter::get_min() const {
    return source_event.get_parameter_description(id).minimum;
}

float AudioParameter::get_max() const {
    return source_event.get_parameter_description(id).maximum;
}

bool VolumeControl::is_valid() const {
    if (*this == VolumeControl {}) return false;  // Check if the volume control has an invalid uuid (in which case it, itself is invalid).

    const FMOD::Studio::VCA* vca = engine.audio.get_vca(uuid);
    return source_bank && vca->isValid();
}

std::string VolumeControl::get_path() const {
#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG)
    if (TryLogError(!is_valid(), "Invalid VolumeControl for get_path")) return "";

    const FMOD::Studio::VCA* vca = engine.audio.get_vca(uuid);

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
    if (TryLogError(!is_valid(), "Invalid VolumeControl for get_volume")) return 0.0f;

    const FMOD::Studio::VCA* vca = engine.audio.get_vca(uuid);

    float volume = 0.0f;
    const FMOD_RESULT result = vca->getVolume(&volume);
    if (TryLogError(result, "Failed to get VCA volume")) return 0.0f;

    return volume;
}

void VolumeControl::set_volume(const float volume) const {
    if (TryLogError(!is_valid(), "Invalid VolumeControl for set_volume")) return;

    FMOD::Studio::VCA* vca = engine.audio.get_vca(uuid);

    const FMOD_RESULT result = vca->setVolume(volume);
    TryLogError(result, "Failed to set VCA volume");
}

void Audio::init() {
    Log::info("Audio Engine: FMOD Studio by Firelight Technologies Pty Ltd.");

    FMOD_RESULT result = FMOD::Studio::System::create(&system);
    if (TryLogError(result, "Failed to create the FMOD Studio System")) return;

    // Initialize FMOD Studio, which will also initialize FMOD Core.
    result = system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (TryLogError(result, "Failed to initialize the FMOD Studio System")) return;

    result = system->getCoreSystem(&core_system);
    TryLogError(result, "Failed to get the FMOD Studio System after initialization");

    // Setup delegates to handle adding and removing listener's to FMOD when an AudioListener components gets added or destroyed (constructor/deconstructors can't be used, these might be
    // called when the registry moves components).
    engine.ecs.get_registry().on_construct<AudioListener>().connect<&Audio::add_listener>(this);
    engine.ecs.get_registry().on_destroy<AudioListener>().connect<&Audio::remove_listener>(this);
}

void Audio::update() {
    // Game components should only update their position if the game is running and not paused.
    if (engine.game_controller.is_playing() && not engine.game_controller.is_paused()) {
        update_listeners();
        update_emitters();
    }

    // Audio updates should always keep updating, this makes sure we can preview sounds in the editor.
    system->update();

    std::erase_if(active_instances, [](const FMOD::Studio::EventInstance* instance) {
        FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
        instance->getPlaybackState(&state);
        return state == FMOD_STUDIO_PLAYBACK_STOPPED;
    });
}

void Audio::end() const {
    // Remove delegates.
    engine.ecs.get_registry().on_destroy<AudioListener>().disconnect<&Audio::remove_listener>(this);
    engine.ecs.get_registry().on_construct<AudioListener>().disconnect<&Audio::add_listener>(this);

    system->release();
    core_system->release();
}

void Audio::on_game_pause() {
    paused_game_audio.reserve(active_instances.size());

    for (AudioInstance instance : active_instances) {
        if (instance.get_paused()) continue;

        // If the audio instance is not paused yet, we pause it to resume once the game resumes.
        paused_game_audio.emplace_back(instance);
        instance.set_paused(true);
    }
}

void Audio::on_game_resume() {
    // Resume the stored audio instances then clear the vector.
    for (AudioInstance instance : paused_game_audio) {
        instance.set_paused(false);
    }
    paused_game_audio.clear();
}

void Audio::on_game_end() {
    paused_game_audio.clear();
    stop_all_audio_instances();
}

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

FMOD::Studio::EventDescription* Audio::get_event_description(const FMOD_GUID& guid) const {
    FMOD::Studio::EventDescription* description = nullptr;

    const FMOD_RESULT result = system->getEventByID(&guid, &description);
    TryLogError(result, "Failed to get EventDescription");

    return description;
}

FMOD::Studio::VCA* Audio::get_vca(const FMOD_GUID& guid) const {
    FMOD::Studio::VCA* vca = nullptr;

    const FMOD_RESULT result = system->getVCAByID(&guid, &vca);
    TryLogError(result, "Failed to get VCA");

    return vca;
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

void Audio::set_global_parameter(const AudioParameter& parameter, const int value) const {
    set_global_parameter(parameter, static_cast<float>(value));
}

void Audio::set_global_label_parameter(const AudioParameter& parameter, const std::string& value) const {
    const FMOD_RESULT result = system->setParameterByIDWithLabel(parameter.get_id(), value.c_str());

    TryLogError(result, "Failed to set parameter with label");
}

void Audio::set_3d_settings(const DopplerSettings& settings) const {
    const FMOD_RESULT result = core_system->set3DSettings(settings.doppler_scale, settings.distance_factor, settings.rolloff_scale);

    TryLogError(result, "Failed to set doppler settings");
}

Audio::DopplerSettings Audio::get_3d_settings() const {
    DopplerSettings settings {};
    const FMOD_RESULT result = core_system->get3DSettings(&settings.doppler_scale, &settings.distance_factor, &settings.rolloff_scale);

    TryLogError(result, "Failed to get doppler settings");
    return settings;
}

void Audio::update_listeners() const {
    // Loop over the all listeners to update their positions/velocities.
    const entt::basic_group listener_group = engine.ecs.group<AudioListener>(entt::get<Transform>);
    for (const auto&& [entity, audio_listener, transform] : listener_group.each()) {
        const VoxelBody* voxel_body = engine.ecs.try_get_component<VoxelBody>(entity);

        FMOD_VECTOR velocity {};
        if (voxel_body != nullptr) velocity = std::bit_cast<FMOD_VECTOR>(voxel_body->velocity);  // Set the velocity if the entity also has a VoxelBody component.

        const FMOD_3D_ATTRIBUTES attributes {
            .position = std::bit_cast<FMOD_VECTOR>(transform.get_world_position()),
            .velocity = velocity,
            .forward = std::bit_cast<FMOD_VECTOR>(transform.get_forward()),
            .up = std::bit_cast<FMOD_VECTOR>(transform.get_up()),
        };

        const FMOD_RESULT result = system->setListenerAttributes(audio_listener.get_listener_index(), &attributes);
        TryLogError(result, "Failed to set listener attributes");
    }
}

void Audio::update_emitters() {
    // Loop over the all emitters to update their positions/velocities.
    const entt::basic_group emitter_group = engine.ecs.group<AudioEmitter>(entt::get<Transform>);
    for (const auto&& [entity, audio_emitter, transform] : emitter_group.each()) {
        audio_emitter.cleanup_playing_instances();

        const VoxelBody* voxel_body = engine.ecs.try_get_component<VoxelBody>(entity);

        FMOD_VECTOR velocity {};
        if (voxel_body != nullptr) velocity = std::bit_cast<FMOD_VECTOR>(voxel_body->velocity);  // Set the velocity if the entity also has a VoxelBody component.

        const FMOD_3D_ATTRIBUTES attributes {
            .position = std::bit_cast<FMOD_VECTOR>(transform.get_world_position()),
            .velocity = velocity,
            .forward = std::bit_cast<FMOD_VECTOR>(transform.get_forward()),
            .up = std::bit_cast<FMOD_VECTOR>(transform.get_up()),
        };

        for (const AudioInstance3D& instance : audio_emitter.playing_instances) {
            const FMOD_RESULT result = instance.instance->set3DAttributes(&attributes);
            TryLogError(result, "Failed to set emitter attributes");
        }
    }
}

void Audio::add_listener(entt::registry& registry, const Entity entity) const {
    int fmod_listener_count;
    FMOD_RESULT result = system->getNumListeners(&fmod_listener_count);
    if (TryLogError(result, "Failed to get listener count")) return;

    int listener_index = 0;

    // Only increase the FMOD listener count if the listener count doesn't already match (FMOD always has 1 listener by default).
    const size_t listener_comp_count = engine.ecs.group<AudioListener>(entt::get<Transform>).size();
    if (listener_comp_count != static_cast<size_t>(fmod_listener_count)) {
        listener_index = fmod_listener_count + 1;

        result = system->setNumListeners(listener_index);
        if (TryLogError(result, "Failed to increase listener count")) return;
    }

    AudioListener& audio_listener = registry.get<AudioListener>(entity);
    audio_listener.listener_index = listener_index;  // Set to the old listener count aka, the *new* last index.
}

void Audio::remove_listener() const {
    int fmod_listener_count;
    FMOD_RESULT result = system->getNumListeners(&fmod_listener_count);
    if (TryLogError(result, "Failed to get listener count")) return;

    // FMOD needs 1 listener minimum, if we remove the last listener component, we should keep the 1 FMOD listener and thus return without updating anything.
    if (fmod_listener_count <= 1) return;

    int new_listener_index = 0;
    const entt::basic_group listener_group = engine.ecs.group<AudioListener>(entt::get<Transform>);
    for (const auto&& [entity, audio_listener, transform] : listener_group.each()) {
        const float cached_weight = audio_listener.get_weight();

        audio_listener.listener_index = new_listener_index;
        audio_listener.set_weight(cached_weight);

        ++new_listener_index;
    }

    result = system->setNumListeners(fmod_listener_count - 1);
    TryLogError(result, "Failed to increase listener count");
}

void Audio::on_draw_lines() const {
    const bool game_active = engine.game_controller.is_playing();

    // Loop over the all emitters to draw distance bounds of their sounds.
    const entt::basic_group emitter_group = engine.ecs.group<AudioEmitter>(entt::get<Transform>);
    for (const auto&& [entity, audio_emitter, transform] : emitter_group.each()) {
        const bool will_play_on_start = audio_emitter.play_on_start && audio_emitter.event_on_start.is_valid();

        if (game_active) {
            // During pause or gameplay, draw the distance bounds of the active playing sounds.
            FMOD_3D_ATTRIBUTES attributes;
            for (const AudioInstance3D& instance : audio_emitter.playing_instances) {
                // on_draw_lines() is called before the audio update function, so we have to make sure the instance is still valid.
                if (not instance.is_valid()) continue;

                FMOD_RESULT result = instance.instance->get3DAttributes(&attributes);
                if (TryLogError(result, "Failed to get 3d audio instance attributes")) continue;

                float min_distance;
                float max_distance;
                result = instance.instance->getMinMaxDistance(&min_distance, &max_distance);
                if (TryLogError(result, "Failed to get 3d audio instance min/max distance")) continue;

                engine.polyline.use_line_width(1.5f);
                engine.polyline.use_color(glm::vec3 { 1.0f, 0.0f, 0.0f });
                engine.polyline.draw_sphere(std::bit_cast<glm::vec3>(attributes.position), min_distance);
                engine.polyline.use_color(glm::vec3 { 0.0f, 1.0f, 0.0f });
                engine.polyline.draw_sphere(std::bit_cast<glm::vec3>(attributes.position), max_distance);
            }
        } else if (will_play_on_start) {
            // When not playing the game, draw the distance bounds of the active playing sounds in the viewport.
            const glm::vec2 distance = audio_emitter.event_on_start.get_min_max_distance();
            const glm::vec3 position = transform.get_world_position();

            engine.polyline.use_line_width(1.5f);
            engine.polyline.use_color(glm::vec3 { 1.0f, 0.0f, 0.0f });
            engine.polyline.draw_sphere(position, distance.x);
            engine.polyline.use_color(glm::vec3 { 0.0f, 1.0f, 0.0f });
            engine.polyline.draw_sphere(position, distance.y);
        }
    }
}

}  // namespace tmt