#include "damped_transform.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/systems/animation/components/constraints.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

namespace tmt
{
void AnimConstraints::DampedTransform::set_local_rest_pose(const tmt::Transform& transform, AnimConstraints::LocalRestPose& rest_pose) {
    rest_pose.local_rot = transform.get_local_rotation();
    rest_pose.local_pos = transform.get_local_position();
    rest_pose.prev_world_rot = transform.get_world_rotation();
    rest_pose.prev_world_pos = transform.get_world_position();

    const auto& children = transform.get_children();

    for (auto child : children) {
        const auto& child_transform = engine.ecs.get_component<Transform>(child);

        AnimConstraints::LocalRestPose child_rest_pose;
        child_rest_pose.bone_entity = child;
        rest_pose.children_rest_poses.push_back(child_rest_pose);

        set_local_rest_pose(child_transform, rest_pose.children_rest_poses.back());
    }
}

void AnimConstraints::DampedTransform::update_damped_pose(AnimConstraints::LocalRestPose& local_rest_pose, const glm::vec3& parent_world_pos, const glm::quat& parent_world_rot, float damp) {
    for (auto& child_pose : local_rest_pose.children_rest_poses) {
        glm::quat ws_target_rot = parent_world_rot * child_pose.local_rot;
        glm::vec3 ws_target_pos = parent_world_pos + (ws_target_rot * child_pose.local_pos);

        glm::vec3 new_world_pos = glm::mix(ws_target_pos, child_pose.prev_world_pos, damp);
        glm::quat new_world_rot = glm::slerp(ws_target_rot, child_pose.prev_world_rot, damp);

        glm::quat new_local_rot = glm::inverse(parent_world_rot) * new_world_rot;
        glm::vec3 new_local_pos = glm::inverse(parent_world_rot) * (new_world_pos - parent_world_pos);

        auto& child_transform = tmt::engine.ecs.get_component<Transform>(child_pose.bone_entity);

        child_transform.set_local_rotation(new_local_rot);
        child_transform.set_local_position(new_local_pos);

        child_pose.prev_world_pos = new_world_pos;
        child_pose.prev_world_rot = new_world_rot;

        update_damped_pose(child_pose, new_world_pos, new_world_rot, damp);
    }
}
}