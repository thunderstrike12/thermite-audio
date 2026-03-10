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
    if (directory.relative_path.empty()) return;
    armature_entity = p;
    rig_is_loaded = true;
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

void RigModel::recurse(const tmt::VoxelSceneNode& node, tmt::Entity parent_entity, const glm::mat4& parent_matrix, glm::vec3 armature_pos) {
    glm::mat4 matrix = glm::identity<glm::mat4>();
    tmt::Entity voxel_entity = entt::null;
    for (int j = 0; j < bone_entities.size(); j++) {
        auto& bone_name = engine.ecs.get_component<Name>(bone_entities[j]);
        auto& bone_transf = engine.ecs.get_component<Transform>(bone_entities[j]);
        if (bone_name.name == node.name) {
            voxel_entity = tmt::engine.ecs.create_entity(node.name);
            auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel_entity);
            transform.set_world_matrix(parent_matrix * node.transform);
            transform.set_parent(parent_entity);
            matrix = transform.get_world_matrix();

            auto& voxel_name = engine.ecs.get_component<Name>(voxel_entity);
            auto& voxel_transf = engine.ecs.get_component<Transform>(voxel_entity);

            BoneComp& bone_comp_id = engine.ecs.get_component<BoneComp>(bone_entities[j]);
            Bone& b = data->bones[bone_comp_id.id];

            pivot_entities.push_back(engine.ecs.create_entity());
            auto& pivot_transf = engine.ecs.get_component<Transform>(pivot_entities.back());
            auto& pivot_name = engine.ecs.get_component<Name>(pivot_entities.back());
            pivot_transf.set_parent(bone_entities[j]);

            voxel_transf.set_parent(pivot_entities.back());
            voxel_transf.set_local_position(b.mesh_offset - (bone_transf.get_world_position() - armature_pos));
            pivot_transf.set_local_rotation(glm::inverse(glm::quat_cast(bone_transf.get_world_matrix())));
            pivot_name.name = "pivot";
            break;
        }
    }

    if (node.tree && engine.ecs.valid(voxel_entity)) {
        tmt::VoxelRenderer& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel_entity);
        const tmt::ResourceRef volume { {}, std::make_shared<tmt::VoxelVolume>(node) };
        renderer.resource = volume;
    }

    for (const tmt::VoxelSceneNode& child : node.children) {
        recurse(child, voxel_entity, matrix, armature_pos);
    }
};

void tmt::RigModel::attach_voxel_objects() {
    vox_is_loaded = true;

    // Load the voxel scene and populate entities by recursing through nodes
    auto voxel_file = engine.resources.load_resource<VoxelScene>(vox_path);
    glm::vec3 armature_pos = engine.ecs.get_component<Transform>(armature_entity).get_world_position();

    for (const VoxelSceneNode& root_node : voxel_file->root_nodes) {
        recurse(root_node, entt::null, glm::identity<glm::mat4>(), armature_pos);
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
