#pragma once

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "animation_data.hpp"

struct BoneComp {
    int id = -1;
};

namespace tmt {
class RigModel {
   public:
    RigModel() = default;
    RigModel(const IO::FileLocation& directory, Entity p);
    void init(const IO::FileLocation& directory, Entity p);

    bool rig_is_loaded = false;
    bool vox_is_loaded = false;
    std::string rig_path;
    std::string vox_path;

    float time = 0.05f;
    float animation_speed = 1.0f;
    float transfer_time = 0.0f;
    float transfer_threshold = 0.0f;
    Entity armature_entity = entt::null;
    std::vector<Entity> bone_entities;
    std::vector<Entity> voxel_entities;
    std::vector<Entity> pivot_entities;
    ResourceRef<RigData> data;
    IO::FileLocation file_directory = {};

    // void attach_voxel_objects(const IO::FileLocation& directory);

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

    void animate_translation(Transform& t, Bone& b);
    void animate_rotation(Transform& t, Bone& b);
    void animate_scale(Transform& t, Bone& b);

    enum class State { ANIMATE_LOOP, ANIMATE_ONCE, STATIONARY, TRANSFERRING_TO_LOOP, TRANSFERRING_TO_ONCE, TRANSFERRING_TO_STOP } state = State::STATIONARY;

   private:
    std::string next_animation;
    std::string current_animation;
    std::string name;
    glm::vec3 base = {0, 0, 0};
    Entity transform_entity = entt::null;
    void init_bones(const Bone& b, Entity p);
};

}  // namespace tmt