#include "rig_model.hpp"

#include "engine/core/logger.hpp"
#include "engine/core/components/transform.hpp"
#include "engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/voxel_renderer.hpp"

namespace {}

namespace tmt {

void RigModel::init_bones(const Bone& b, Entity p) {
    if (!engine.ecs.try_get_component<Transform>(p)) {
        Log::warn("Parent entity {} has no Transform!", static_cast<uint32_t>(p));
    }

    const auto bone = engine.ecs.create_entity();

    auto& bone_comp = engine.ecs.add_component<BoneComp>(bone);
    bone_comp.id = b.index;

    auto& trans_comp = engine.ecs.get_component<Transform>(bone);
    trans_comp.set_parent(p);
    trans_comp.set_local_position(b.default_trans.get_local_position());
    trans_comp.set_local_rotation(b.default_trans.get_local_rotation());
    auto& name_comp = engine.ecs.get_component<Name>(bone);
    name_comp.name = b.name;

    bone_entities.push_back(bone);
    for (const int i : b.children) {
        init_bones(data->bones[i], bone);
    }
}

void RigModel::init(const IO::FileLocation& directory, Entity p) {
    if (directory.relative_path.empty()) {
        Log::error("Can't initialize RigModel with invalid file: {}", directory);
        return;
    }

    armature_entity = p;
    data = engine.resources.load_resource<RigData>(directory);

    // file_directory = directory;

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

void RigModel::recurse(const ResourceRef<VoxelScene>& scene, const VoxelSceneNode& node, const glm::vec3& armature_pos) {
    std::vector<Entity> pivot_entities;

    Entity voxel_entity = entt::null;
    for (const auto& bone_entity : bone_entities) {
        auto& bone_name = engine.ecs.get_component<Name>(bone_entity);
        const auto& bone_transform = engine.ecs.get_component<Transform>(bone_entity);

        if (bone_name.name == node.name) {
            const BoneComp& bone_comp_id = engine.ecs.get_component<BoneComp>(bone_entity);
            const Bone& bone = data->bones[bone_comp_id.id];

            const Entity pivot_entity = pivot_entities.emplace_back(engine.ecs.create_entity("pivot"));
            auto& pivot_transform = engine.ecs.get_component<Transform>(pivot_entity);
            pivot_transform.set_parent(bone_entity);
            pivot_transform.set_local_rotation(glm::inverse(glm::quat_cast(bone_transform.get_world_matrix())));

            voxel_entity = engine.ecs.create_entity(node.name);
            auto& voxel_transform = engine.ecs.get_component<Transform>(voxel_entity);
            voxel_transform.set_parent(pivot_entity);
            voxel_transform.set_world_matrix(scene->get_node_world_matrix(node.uuid));
            voxel_transform.set_local_position(bone.mesh_offset - (bone_transform.get_world_position() - armature_pos));
            break;
        }
    }

    if (node.tree && engine.ecs.valid(voxel_entity)) {
        VoxelRenderer& renderer = engine.ecs.add_component<VoxelRenderer>(voxel_entity);
        renderer.resource = engine.resources.copy_resource<VoxelVolume>(scene, node.uuid);
    }

    for (const VoxelSceneNode& child : node.children) {
        recurse(scene, child, armature_pos);
    }
};

void RigModel::attach_voxel_objects() {
    if (!VoxelScene::SUPPORTED_FILE_EXTENSIONS.contains(vox_path.relative_path.extension().generic_string())) {
        Log::error("Failed to attach voxel objects, file isn't a voxel file: {}", vox_path);
        return;
    }

    // Load the voxel scene and populate entities by recursing through nodes
    const auto voxel_file = engine.resources.load_resource<VoxelScene>(vox_path);
    const glm::vec3 armature_pos = engine.ecs.get_component<Transform>(armature_entity).get_world_position();

    for (const VoxelSceneNode& root_node : voxel_file->root_nodes) {
        recurse(voxel_file, root_node, armature_pos);
    }

    vox_is_loaded = true;
}

RigModel::RigModel(const IO::FileLocation&, Entity) {}

std::string tmt::RigModel::current_animation_playing() {
    switch (state) {
        case tmt::RigModel::State::ANIMATE_LOOP:
        case tmt::RigModel::State::ANIMATE_ONCE:
            return current_animation;

        case tmt::RigModel::State::TRANSFERRING_TO_LOOP:
        case tmt::RigModel::State::TRANSFERRING_TO_ONCE:
            return next_animation;

        case tmt::RigModel::State::STATIONARY:
        case tmt::RigModel::State::TRANSFERRING_TO_STOP:
            return "";
    }
    return "";
}

void RigModel::play_animation(const std::string& animation_name, float _transfer_time, bool loop_animation) {
    const auto& animations = data->bones.back().animations;
    if (!animations.contains(animation_name)) {
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

void RigModel::animate_translation(Pose& t, Bone& b) const {
    const auto& keyframes = b.animations[get_current_animation()].keyframes_pos;
    if (keyframes.empty()) return;

    int current_frame = (int)keyframes.size() - 1;
    for (int i = 0; i < (int)keyframes.size(); i++) {
        if (time < keyframes[i].time) {
            current_frame = std::max(i - 1, 0);
            break;
        }
    }
    if (current_frame >= (int)keyframes.size() - 1) {
        t.translation = (keyframes[current_frame].position);
        return;
    }

    const float current_frame_time = keyframes[current_frame].time;
    const float next_frame_time = keyframes[current_frame + 1].time;
    float interp_time = (time - current_frame_time) / (next_frame_time - current_frame_time);

    glm::vec3 new_position = glm::mix(keyframes[current_frame].position, keyframes[current_frame + 1].position, interp_time);

    // transfer from current frame to first frame of new animation
    if (is_transferring()) {
        interp_time = transfer_time / transfer_threshold;
        new_position = glm::mix(b.animations[get_current_animation()].keyframes_pos[current_frame].position, b.animations[get_next_animation()].keyframes_pos[0].position, interp_time);
        new_position = glm::mix(glm::mix(t.translation, b.animations[get_next_animation()].keyframes_pos[0].position, interp_time), new_position, 0.4f);
    }

    t.translation = (new_position);
}

void RigModel::animate_rotation(Pose& t, Bone& b) const {
    const auto& keyframes = b.animations[get_current_animation()].keyframes_rot;
    if (keyframes.empty()) return;

    int current_frame = (int)keyframes.size() - 1;
    for (int i = 0; i < (int)keyframes.size(); i++) {
        if (time < keyframes[i].time) {
            current_frame = std::max(i - 1, 0);
            break;
        }
    }

    if (current_frame >= (int)keyframes.size() - 1) {
        t.rotation = (keyframes[current_frame].rotation);
        return;
    }

    const float current_frame_time = keyframes[current_frame].time;
    const float next_frame_time = keyframes[std::min(current_frame + 1, (int)keyframes.size() - 1)].time;
    float interp_time = (time - current_frame_time) / (next_frame_time - current_frame_time);

    glm::quat new_rotation = glm::slerp(keyframes[current_frame].rotation, keyframes[std::min(current_frame + 1, (int)keyframes.size() - 1)].rotation, interp_time);

    // transfer from current frame to first frame of new animation
    if (is_transferring()) {
        interp_time = transfer_time / transfer_threshold;
        new_rotation = glm::slerp(b.animations[get_current_animation()].keyframes_rot[current_frame].rotation, b.animations[get_next_animation()].keyframes_rot[0].rotation, interp_time);
        new_rotation = glm::slerp(glm::slerp(t.rotation, b.animations[get_next_animation()].keyframes_rot[0].rotation, interp_time), new_rotation, 0.4f);
    }

    assert(!glm::any(glm::isnan(new_rotation)));
    t.rotation = (new_rotation);
}

void RigModel::animate_scale(Pose& t, Bone& b) const {
    const auto& keyframes = b.animations[get_current_animation()].keyframes_scale;
    if (keyframes.empty()) return;

    int current_frame = (int)keyframes.size() - 1;
    for (int i = 0; i < (int)keyframes.size(); i++) {
        if (time < keyframes[i].time) {
            current_frame = std::max(i - 1, 0);
            break;
        }
    }

    if (current_frame >= (int)keyframes.size() - 1) {
        t.scale = (keyframes[current_frame].scale);
        return;
    }

    const float current_frame_time = keyframes[current_frame].time;
    const float next_frame_time = keyframes[current_frame + 1].time;
    float interp_time = (time - current_frame_time) / (next_frame_time - current_frame_time);

    glm::vec3 new_scale = glm::mix(keyframes[current_frame].scale, keyframes[current_frame + 1].scale, interp_time);

    // transfer from current frame to first frame of new animation
    if (is_transferring()) {
        interp_time = transfer_time / transfer_threshold;
        new_scale = glm::mix(t.scale, b.animations[get_next_animation()].keyframes_scale[0].scale, interp_time);
    }

    t.scale = (new_scale);
}

void ConstrainedRig::copy_reference_pose_from_keyframe(RigModel& rig_model) {
    for (auto& [ent, pose] : constrained_poses) {
        pose = rig_model.bone_keyframes[ent];
    }
}

void ConstrainedRig::restore_reference_poses_from_keyframe() {
    for (auto& [ent, pose] : constrained_poses) {
        pose = initial_reference_poses[ent];
    }
}

}  // namespace tmt
