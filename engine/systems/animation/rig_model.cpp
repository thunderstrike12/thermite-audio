#include "rig_model.hpp"

#include "engine/core/logger.hpp"
#include "engine/core/components/transform.hpp"
#include "engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"

namespace tmt {

void RigModel::init_bones(const Bone& b, Entity p) {
    Log::info("init bones got called!");
    if (!engine.ecs.try_get_component<Transform>(p)) {
        Log::warn("Parent entity {} has no Transform!", static_cast<uint32_t>(p));
    }

    auto bone = engine.ecs.create_entity();

    auto& bone_comp = engine.ecs.add_component<BoneComp>(bone);
    bone_comp.id = b.index;

    auto& trans_comp = engine.ecs.get_component<Transform>(bone);
    trans_comp.set_parent(p);
    trans_comp.set_local_position(b.default_trans.get_local_position());
    trans_comp.set_local_rotation(b.default_trans.get_local_rotation());
    auto& name_comp = engine.ecs.get_component<Name>(bone);
    name_comp.name = b.name;

    bone_entities.push_back(bone);
    for (int i = 0; i < b.children.size(); i++) {
        init_bones(data->bones[b.children[i]], bone);
    }
}

void RigModel::init(const IO::FileLocation& directory, Entity p) {
    armature_entity = p;
    rig_is_loaded = true;
    data = engine.resources.load_resource<RigData>(directory);

    file_directory = directory;

    name = data->name;

    if (bone_entities.empty()) {
        init_bones(data->bones.back(), p);
    }

    if (p != entt::null) {
        const auto& animations = data->bones.back().animations;
        if (!animations.empty()) current_animation = animations.begin()->first;

        auto& compt = engine.ecs.get_component<Transform>(p);
        auto& compn = engine.ecs.get_component<Name>(p);
        compn.name = name;
        auto trans = compt.get_local_position();
        compt.set_local_position({ trans.x, trans.y, trans.z });
    }
}

RigModel::RigModel(const IO::FileLocation&, Entity) {}

std::string tmt::RigModel::current_animation_playing() {
    switch (state) {
        case tmt::RigModel::State::ANIMATE_LOOP:
            return current_animation;
            break;
        case tmt::RigModel::State::ANIMATE_ONCE:
            return current_animation;
            break;
        case tmt::RigModel::State::STATIONARY:
            return "";
            break;
        case tmt::RigModel::State::TRANSFERRING_TO_LOOP:
            return next_animation;
            break;
        case tmt::RigModel::State::TRANSFERRING_TO_ONCE:
            return next_animation;
            break;
        case tmt::RigModel::State::TRANSFERRING_TO_STOP:
            return "";
            break;
        default:
            break;
    }
    return "";
}

void RigModel::play_animation(const std::string& animation_name, float _transfer_time, bool loop_animation) {
    const auto& animations = data->bones[0].animations;
    if (animations.find(animation_name) == animations.end()) {
        Log::warn("invalid animation name: ", animation_name);

        return;
    }
    if (loop_animation)
        state = State::TRANSFERRING_TO_LOOP;
    else
        state = State::TRANSFERRING_TO_ONCE;
    transfer_threshold = _transfer_time;
    transfer_time = 0.0f;
    next_animation = animation_name;
}

void RigModel::stop_loop(float transfer_time_t) {
    state = RigModel::State::TRANSFERRING_TO_STOP;
    transfer_threshold = transfer_time_t;
    transfer_time = 0.0f;
}

void tmt::RigModel::set_animation_speed(float s) {
    animation_speed = s;
}

void RigModel::animate_translation(Transform& t, Bone& b) {
    auto& keyframes = b.animations[get_current_animation()].keyframes_pos;
    int current_frame = (int)keyframes.size() - 1;
    for (int i = 0; i < keyframes.size(); i++) {
        if (time < keyframes[i].time) {
            current_frame = std::max(i - 1, 0);
            break;
        }
    }
    if (current_frame >= keyframes.size() - 1)
        t.set_local_position(keyframes[current_frame].position);
    else {
        float current_frame_time = keyframes[current_frame].time;
        float next_frame_time = keyframes[current_frame + 1].time;
        float interp_time = (time - current_frame_time) / (next_frame_time - current_frame_time);

        glm::vec3 new_position = glm::mix(keyframes[current_frame].position, keyframes[current_frame + 1].position, interp_time);

        // transfer from current frame to first frame of new animation
        if (is_transferring()) {
            interp_time = transfer_time / transfer_threshold;
            new_position = glm::mix(b.animations[get_current_animation()].keyframes_pos[current_frame].position, b.animations[get_next_animation()].keyframes_pos[0].position, interp_time);
            new_position = glm::mix(glm::mix(t.get_local_position(), b.animations[get_next_animation()].keyframes_pos[0].position, interp_time), new_position, 0.4f);
        }

        t.set_local_position(new_position);
    }
}

void RigModel::animate_rotation(Transform& t, Bone& b) {
    auto& keyframes = b.animations[get_current_animation()].keyframes_rot;
    int current_frame = (int)keyframes.size() - 1;
    for (int i = 0; i < keyframes.size(); i++) {
        if (time < keyframes[i].time) {
            current_frame = std::max(i - 1, 0);
            break;
        }
    }
    float current_frame_time = keyframes[current_frame].time;
    float next_frame_time = keyframes[std::min(current_frame + 1, (int)keyframes.size() - 1)].time;
    float interp_time = (time - current_frame_time) / (next_frame_time - current_frame_time);

    glm::quat new_rotation = glm::slerp(keyframes[current_frame].rotation, keyframes[std::min(current_frame + 1, (int)keyframes.size() - 1)].rotation, interp_time);

    // transfer from current frame to first frame of new animation
    if (is_transferring()) {
        interp_time = transfer_time / transfer_threshold;
        new_rotation = glm::slerp(b.animations[get_current_animation()].keyframes_rot[current_frame].rotation, b.animations[get_next_animation()].keyframes_rot[0].rotation, interp_time);
        new_rotation = glm::slerp(glm::slerp(t.get_local_rotation(), b.animations[get_next_animation()].keyframes_rot[0].rotation, interp_time), new_rotation, 0.4f);
    }

    t.set_local_rotation(new_rotation);
}

void RigModel::animate_scale(Transform& t, Bone& b) {
    auto& keyframes = b.animations[get_current_animation()].keyframes_scale;
    int current_frame = (int)keyframes.size() - 1;
    for (int i = 0; i < keyframes.size(); i++) {
        if (time < keyframes[i].time) {
            current_frame = std::max(i - 1, 0);
            break;
        }
    }
    if (current_frame >= keyframes.size() - 1)
        t.set_local_scale(keyframes[current_frame].scale);
    else {
        float current_frame_time = keyframes[current_frame].time;
        float next_frame_time = keyframes[current_frame + 1].time;
        float interp_time = (time - current_frame_time) / (next_frame_time - current_frame_time);

        glm::vec3 new_scale = glm::mix(keyframes[current_frame].scale, keyframes[current_frame + 1].scale, interp_time);

        // transfer from current frame to first frame of new animation
        if (is_transferring()) {
            interp_time = transfer_time / transfer_threshold;
            new_scale = glm::mix(t.get_local_scale(), b.animations[get_next_animation()].keyframes_scale[0].scale, interp_time);
        }

        t.set_local_scale(new_scale);
    }
}

}  // namespace tmt
