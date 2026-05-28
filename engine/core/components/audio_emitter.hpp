#pragma once

#include "engine/core/audio.hpp"
#include "engine/core/reflection.hpp"
#include "engine/events/scene.hpp"

namespace tmt {

class AudioEmitter : public OnSceneStart {
    friend class Audio;

   public:
    bool play_on_start { false };
    AudioEvent event_on_start;

    /// Play the provided AudioEvent in 3D space using this emitter.
    /// @param event: The AudioEvent to play.
    /// @param update_position: If the position and velocity of the sound in space should be updated based on the entity of the AudioEmitter.
    /// @param stop_other_instances: If all other sounds being played by this AudioEmitter should be stopped.
    /// @return The AudioInstance3D of the audio event, can be used to modify parameters while the sound is playing.
    AudioInstance3D play(const AudioEvent& event, bool update_position = true, bool stop_other_instances = false);

    AudioInstance play_2d(const AudioEvent& event, bool stop_other_instances = false);

   private:
    std::vector<AudioInstance3D> playing_instances;
    std::vector<AudioInstance> playing_instances_2d;

    void cleanup_playing_instances();
    void on_scene_start() override;
};

}  // namespace tmt

TMT_COMPONENT(tmt::AudioEmitter, "Audio Emitter", (play_on_start, event_on_start));