#pragma once

#include <set>
#include <string>

#include "fmod_studio_common.h"

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

class AudioParameter {
   public:
    AudioParameter(const FMOD_STUDIO_PARAMETER_DESCRIPTION& description) : description {description} {}

    // Get the name of the AudioParameter.
    [[nodiscard]] std::string get_name() const { return description.name; }
    [[nodiscard]] const FMOD_STUDIO_PARAMETER_ID& get_id() const { return description.id; }
    [[nodiscard]] float get_min() const { return description.minimum; }
    [[nodiscard]] float get_max() const { return description.maximum; }

   private:
    FMOD_STUDIO_PARAMETER_DESCRIPTION description;
};

class AudioInstance {
   public:
    AudioInstance(FMOD::Studio::EventInstance* instance) : instance {instance} {}

    [[nodiscard]] bool is_valid() const;
    void stop(FMOD_STUDIO_STOP_MODE stop_mode = FMOD_STUDIO_STOP_IMMEDIATE) const;
    void set_parameter(const AudioParameter& event_parameter, float value) const;
    void set_parameter(const AudioParameter& event_parameter, int value) const;
    void set_label_parameter(const AudioParameter& event_parameter, const std::string& value) const;

   private:
    FMOD::Studio::EventInstance* instance {nullptr};
};

class AudioEvent {
   public:
    AudioEvent(FMOD::Studio::EventDescription* description) : description {description} {}

    [[nodiscard]] bool is_valid() const;
    // Get the path/name of the AudioEvent (returns empty string when not in debug mode and not using the editor).
    [[nodiscard]] std::string get_path() const;

    [[nodiscard]] std::vector<AudioParameter> get_parameters() const;
    AudioInstance play() const;

    [[nodiscard]] bool operator==(const AudioEvent& other) const { return description == other.description; }
    [[nodiscard]] bool operator!=(const AudioEvent& other) const { return description != other.description; }

   private:
    FMOD::Studio::EventDescription* description {nullptr};
};

class VolumeControl {
   public:
    VolumeControl(FMOD::Studio::VCA* vca) : vca {vca} {}

    [[nodiscard]] bool is_valid() const;
    // Get the path/name of the VolumeControl (returns empty string when not in debug mode and not using the editor).
    [[nodiscard]] std::string get_path() const;

    [[nodiscard]] float get_volume() const;
    void set_volume(float volume) const;

    [[nodiscard]] bool operator==(const VolumeControl& other) const { return vca == other.vca; }
    [[nodiscard]] bool operator!=(const VolumeControl& other) const { return vca != other.vca; }

   private:
    FMOD::Studio::VCA* vca {nullptr};
};

class Audio {
    friend class AudioParameter;
    friend class AudioInstance;
    friend class AudioEvent;

   public:
    Audio() = default;
    ~Audio() = default;

    void init();
    void update();
    void end() const;

    // Initialize an FMOD bank with all its assets from a .bank file in memory.
    [[nodiscard]] FMOD::Studio::Bank* init_bank(const std::vector<char>& bank_data) const;

    [[nodiscard]] AudioEvent get_event(const FMOD_GUID& guid) const;
    [[nodiscard]] VolumeControl get_volume_control(const FMOD_GUID& guid) const;

    void stop_all_audio_instances();
    void set_global_parameter(const AudioParameter& parameter, float value) const;
    void set_global_parameter(const AudioParameter& parameter, int value) const;
    void set_global_label_parameter(const AudioParameter& parameter, const std::string& value) const;

   private:
    FMOD::Studio::System* system;
    FMOD::System* core_system;

    std::set<FMOD::Studio::EventInstance*> active_instances;
};

}  // namespace tmt