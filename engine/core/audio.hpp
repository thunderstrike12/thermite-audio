#pragma once

#include <set>
#include <string>

#include <entt/entity/fwd.hpp>
#include <fmod_studio_common.h>

#include "engine/events/game.hpp"
#include "engine/core/resource.hpp"
#include "engine/events/debug.hpp"

struct FMOD_GUID;
struct FMOD_STUDIO_PARAMETER_ID;

// NOLINTBEGIN(readability-identifier-naming)
namespace FMOD {

class System;

namespace Studio {

class VCA;
class Bank;
class System;
class EventInstance;
class EventDescription;

}  // namespace Studio

}  // namespace FMOD
// NOLINTEND(readability-identifier-naming)

namespace tmt {

class AudioBank;
struct Transform;
struct VoxelBody;

class AudioParameter {
   public:
    AudioParameter(const FMOD_STUDIO_PARAMETER_DESCRIPTION& description) : description { description } {}

    // Get the name of the AudioParameter.
    [[nodiscard]] std::string get_name() const { return description.name; }
    [[nodiscard]] const FMOD_STUDIO_PARAMETER_ID& get_id() const { return description.id; }
    [[nodiscard]] float get_min() const { return description.minimum; }
    [[nodiscard]] float get_max() const { return description.maximum; }

   private:
    FMOD_STUDIO_PARAMETER_DESCRIPTION description;
};

class AudioEvent;

class AudioInstance {
    friend class AudioEvent;

   public:
    AudioInstance(FMOD::Studio::EventInstance* instance) : instance { instance } {}

    [[nodiscard]] bool is_valid() const;

    void stop(FMOD_STUDIO_STOP_MODE stop_mode = FMOD_STUDIO_STOP_IMMEDIATE) const;
    void set_paused(bool pause = true) const;
    [[nodiscard]] bool get_paused() const;
    void toggle_paused() const { set_paused(!get_paused()); }

    void set_parameter(const AudioParameter& event_parameter, float value) const;
    void set_parameter(const AudioParameter& event_parameter, int value) const;
    void set_label_parameter(const AudioParameter& event_parameter, const std::string& value) const;

   protected:
    FMOD::Studio::EventInstance* instance { nullptr };
};

class AudioInstance3D : public AudioInstance {
    friend class Audio;

    using AudioInstance::AudioInstance;

   public:
    void set_3d_position(const glm::vec3& position) const;
    // Set the velocity of the sound, used to calculate the sound's doppler effect.
    void set_3d_velocity(const glm::vec3& velocity) const;
    void set_3d_forward(const glm::vec3& forward) const;
    void set_3d_up(const glm::vec3& up) const;
    /// Automatically set the attributes of the instance based on the transform and the voxel_body_velocity.
    /// @param transform: The transform used for the instance's 3D position, up, and forward vector in the world.
    /// @param voxel_body: Optional VoxelBody pointer used for the velocity (used to calculate the doppler effect), default value nullptr will set velocity to {0, 0, 0}.
    void auto_set_3d_attributes(const Transform& transform, const VoxelBody* voxel_body = nullptr) const;

    // Set the bitmask of which listeners should listen to this sound instance (default is all on).
    void set_listener_mask(unsigned int mask) const;
    // Get the bitmask of which listeners should listen to this sound instance (default is all on).
    unsigned int get_listener_mask() const;

   private:
    [[nodiscard]] FMOD_3D_ATTRIBUTES get_3d_attributes() const;
};

class AudioEvent {
   public:
    AudioEvent() = default;
    AudioEvent(const ResourceRef<AudioBank>& source_bank, const FMOD_GUID& description_uuid) : source_bank { source_bank }, uuid { description_uuid } {}

    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] bool is_3d() const;
    // Get the path/name of the AudioEvent (returns empty string when not in debug mode and not using the editor).
    [[nodiscard]] std::string get_path() const;
    [[nodiscard]] FMOD_GUID get_guid() const { return uuid; }
    [[nodiscard]] const ResourceRef<AudioBank>& get_source_bank() const { return source_bank; }
    [[nodiscard]] std::vector<AudioParameter> get_parameters() const;

    AudioInstance play() const;
    AudioInstance3D play_3d() const;

    [[nodiscard]] glm::vec2 get_min_max_distance() const;

    [[nodiscard]] bool operator==(const AudioEvent& other) const { return std::memcmp(&uuid, &other.uuid, sizeof(FMOD_GUID)) == 0; }
    [[nodiscard]] bool operator!=(const AudioEvent& other) const { return std::memcmp(&uuid, &other.uuid, sizeof(FMOD_GUID)) != 0; }

   private:
    BEFRIEND_VISITABLE()

    ResourceRef<AudioBank> source_bank;
    FMOD_GUID uuid {};
};

class VolumeControl {
   public:
    VolumeControl() = default;
    VolumeControl(const ResourceRef<AudioBank>& source_bank, const FMOD_GUID& vca_uuid) : source_bank { source_bank }, uuid { vca_uuid } {}

    [[nodiscard]] bool is_valid() const;
    // Get the path/name of the VolumeControl (returns empty string when not in debug mode and not using the editor).
    [[nodiscard]] std::string get_path() const;
    [[nodiscard]] FMOD_GUID get_guid() const { return uuid; }
    [[nodiscard]] const ResourceRef<AudioBank>& get_source_bank() const { return source_bank; }

    [[nodiscard]] float get_volume() const;
    void set_volume(float volume) const;

    [[nodiscard]] bool operator==(const VolumeControl& other) const { return std::memcmp(&uuid, &other.uuid, sizeof(FMOD_GUID)) == 0; }
    [[nodiscard]] bool operator!=(const VolumeControl& other) const { return std::memcmp(&uuid, &other.uuid, sizeof(FMOD_GUID)) != 0; }

   private:
    BEFRIEND_VISITABLE()

    ResourceRef<AudioBank> source_bank;
    FMOD_GUID uuid {};
};

class AudioListener;
using Entity = entt::entity;

class Audio : public OnGamePause, public OnGameResume, public OnGameEnd, public OnDrawLines {
    friend class AudioParameter;
    friend class AudioInstance;
    friend class AudioEvent;
    friend class Engine;
    friend class AudioListener;

   public:
    struct DopplerSettings {
        float doppler_scale { 1.0f };
        float distance_factor { 1.0f };
        float rolloff_scale { 1.0f };
    };

    Audio() = default;
    ~Audio() override = default;

    void init();
    void update();
    void end() const;

    // Initialize an FMOD bank with all its assets from a .bank file in memory.
    [[nodiscard]] FMOD::Studio::Bank* init_bank(const std::vector<char>& bank_data) const;

    [[nodiscard]] FMOD::Studio::EventDescription* get_event_description(const FMOD_GUID& guid) const;
    [[nodiscard]] FMOD::Studio::VCA* get_vca(const FMOD_GUID& guid) const;

    void stop_all_audio_instances();
    void set_global_parameter(const AudioParameter& parameter, float value) const;
    void set_global_parameter(const AudioParameter& parameter, int value) const;
    void set_global_label_parameter(const AudioParameter& parameter, const std::string& value) const;

    void set_3d_settings(const DopplerSettings& settings) const;
    [[nodiscard]] DopplerSettings get_3d_settings() const;

   private:
    void update_listeners() const;
    static void update_emitters();

    // Used to make sure pausing/ending the game doesn't keep playing the game's audio.
    void on_game_pause() override;
    void on_game_resume() override;
    void on_game_end() override;

    void add_listener(entt::registry& registry, Entity entity) const;
    void remove_listener() const;

    FMOD::Studio::System* system;
    FMOD::System* core_system;

    std::set<FMOD::Studio::EventInstance*> active_instances;

    // Inherited from OnDrawLines
    void on_draw_lines() const override;
    constexpr std::string get_name() const override { return "Audio Distance Bounds"; }
};

}  // namespace tmt
