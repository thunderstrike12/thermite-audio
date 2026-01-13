#include "audio_emitter.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"

namespace tmt {

inline AudioInstance3D AudioEmitter::play(bool update_position, bool stop_other_instances) {
    if (stop_other_instances) {
        for (AudioInstance3D& instance : playing_instances) {
            instance.stop();
        }
        playing_instances.clear();
    }

    const AudioInstance3D instance = event.play_3d();
    const Entity self = engine.ecs.get_entity(*this);

    // Set up the initial position and potential velocity of the sound.
    const Transform& transform = engine.ecs.get_component<Transform>(self);
    const VoxelBody* voxel_body = engine.ecs.try_get_component<VoxelBody>(self);
    instance.auto_set_3d_attributes(transform, voxel_body);

    if (update_position) playing_instances.push_back(instance);

    return instance;
}

void AudioEmitter::cleanup_playing_instances() {
    std::erase_if(playing_instances, [](const AudioInstance3D& instance) { return !instance.is_valid(); });
}

void AudioEmitter::on_scene_start() {
    if (play_on_start && event.is_valid()) playing_instances.push_back(play(true));
}

}  // namespace tmt