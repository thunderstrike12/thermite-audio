#include "animation_player.hpp"
#include "engine/core/logger.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/animation/rig_model.hpp"

void AnimationPlayer::start() {
    auto& rigmodel = tmt::engine.ecs.add_component<tmt::RigModel>(entity);
    rigmodel.init(rig_location, entity);
    for (const auto& animation : animation_location) {
        rigmodel.data->animation_files.push_back(animation);
    }
    rigmodel.data->reload();
    rigmodel.play_animation(animation_name, 0.0f, true);
}

void AnimationPlayer::update(const tmt::FrameData&) {}

void AnimationPlayer::end() {}
