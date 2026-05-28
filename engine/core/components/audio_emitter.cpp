#include "audio_emitter.hpp"

#include "engine/core/ecs.hpp"

namespace tmt {

AudioInstance3D AudioEmitter::play(const AudioEvent& event, const bool update_position, const bool stop_other_instances) {
    if (stop_other_instances) {
        for (AudioInstance3D& instance : playing_instances) {
            instance.stop();
        }
        playing_instances.clear();
    }

    const AudioInstance3D instance = event.play_3d();
    if (update_position) playing_instances.push_back(instance);

    return instance;
}

AudioInstance AudioEmitter::play_2d(const AudioEvent& event, const bool stop_other_instances) {
    if (stop_other_instances) {
        for (AudioInstance& instance : playing_instances_2d) {
            instance.stop();
        }
        playing_instances_2d.clear();
    }

    const AudioInstance instance = event.play();

    return instance;
}

void AudioEmitter::cleanup_playing_instances() {
    std::erase_if(playing_instances, [](const AudioInstance3D& instance) { return !instance.is_valid(); });
}

void AudioEmitter::on_scene_start() {
    if (play_on_start && event_on_start.is_valid()) {
        if (event_on_start.is_3d())
            playing_instances.push_back(play(event_on_start, true));
        else
            playing_instances_2d.push_back(play_2d(event_on_start));
    }
}

}  // namespace tmt