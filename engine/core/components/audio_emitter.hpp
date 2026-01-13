#pragma once

#include "engine/core/audio.hpp"
#include "engine/core/reflection.hpp"
#include "engine/events/scene.hpp"

namespace tmt {

class AudioEmitter : public OnSceneStart {
    friend class Audio;

   public:
    bool play_on_start {false};
    AudioEvent event;

    AudioInstance3D play(bool update_position = true, bool stop_other_instances = false);

   private:
    std::vector<AudioInstance3D> playing_instances;

    void cleanup_playing_instances();
    void on_scene_start() override;
};

}  // namespace tmt

TMT_COMPONENT(tmt::AudioEmitter, "Audio Emitter", (play_on_start, event));