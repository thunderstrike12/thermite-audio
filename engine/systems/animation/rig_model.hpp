#pragma once

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "animation_data.hpp"

#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

struct BoneComp {
    int id = -1;
};

class RigModel {
   public:
    RigModel() = default;
    RigModel(const IO::FileLocation& directory, Entity p);
    // todo: change this to Resource ref
    void init(const IO::FileLocation& directory, Entity p);

    bool vox_is_loaded = false;
    IO::FileLocation vox_path = {};
    ResourceRef<RigData> data;
    std::vector<Entity> bone_entities;

    float time = 0.05f;
    float animation_speed = 1.0f;
    float transfer_time = 0.0f;
    float transfer_threshold = 0.0f;
    Entity armature_entity = entt::null;

    void recurse(const ResourceRef<VoxelScene>& scene, const tmt::VoxelSceneNode& node, const glm::mat4& parent_matrix, const glm::vec3& armature_pos);
    void attach_voxel_objects();

    [[nodiscard]] bool is_transferring() const {
        if (state == State::TRANSFERRING_TO_LOOP || state == State::TRANSFERRING_TO_ONCE || state == State::TRANSFERRING_TO_STOP) return true;

        return false;
    }

    void play_animation(const std::string& animation_name, float transfer_time, bool loop_animation);
    void stop_loop(float transfer_time_t);
    void set_animation_speed(float s);

    void set_current_animation(std::string v) { current_animation = std::move(v); }
    std::string get_current_animation() const { return current_animation; }
    std::string get_next_animation() const { return next_animation; }
    std::string current_animation_playing();

    void animate_translation(Transform& t, Bone& b) const;
    void animate_rotation(Transform& t, Bone& b) const;
    void animate_scale(Transform& t, Bone& b) const;

    enum class State : uint8_t { ANIMATE_LOOP, ANIMATE_ONCE, STATIONARY, TRANSFERRING_TO_LOOP, TRANSFERRING_TO_ONCE, TRANSFERRING_TO_STOP } state = State::STATIONARY;

   private:
    std::string next_animation;
    std::string current_animation;
    std::string name;
    glm::vec3 base = { 0, 0, 0 };
    Entity transform_entity = entt::null;
    void init_bones(const Bone& b, Entity p);
};

}  // namespace tmt

JSON_REFLECT(tmt::BoneComp, id);
TMT_COMPONENT(tmt::RigModel, "Animated Rig", (vox_is_loaded, vox_path, data));