#pragma once
#include "engine/core/entity.hpp"
#include "glm/gtc/quaternion.hpp"
#include "engine/core/reflection.hpp"
#include "engine/tools/types/bezier_curve.hpp"

namespace tmt {
namespace AnimConstraints {

struct LocalRestPose {
    glm::vec3 local_pos;
    glm::quat local_rot;
    tmt::Entity bone_entity;

    glm::vec3 prev_world_pos;
    glm::quat prev_world_rot;

    std::vector<LocalRestPose> children_rest_poses;
};

struct DampedTransformConstraint {
    float damp = 0.f;
    LocalRestPose local_rest_pose;  // root
};
struct TwoBoneIKConstraint {
    // automatically set
    tmt::Entity parent = entt::null;
    // automatically set
    tmt::Entity root = entt::null;

    tmt::Entity mid = entt::null;
    tmt::Entity tip = entt::null;

    tmt::Entity target_position_entity = entt::null;
    tmt::Entity bend_position_entity = entt::null;

    glm::quat twist_rest_pose_root;
    glm::quat twist_rest_pose_mid;
    glm::quat foot_parent_rest_rotation;

    tmt::Entity constrained_rig_ent = entt::null;
    bool obey_foot_bind_pose = false;
};
struct EffectorWalkCycle {
    // step after how much time?
    float step_time;

    // step motion duration
    float step_duration;

    // step motion height
    float step_height;

    bool grounded;

    // the continuously available position
    tmt::Entity desired_target_entity = entt::null;

    tmt::Entity effector_entity = entt::null;

    tmt::Entity ground_entity = entt::null;

    // the timing offset of the internal step timer
    float cycle_offset = 0.f;

    BezierCurve stepping_curve;
    BezierCurve height_curve;

    float step_prediction_strength = 1.f;

    glm::vec3 last_continuous_pos;
    glm::vec3 initial_pos;
    glm::vec3 desired_pos;

    // the internally tracked effector
    glm::vec3 effector;

    float step_timer = 0.f;
    float step_interp = 0.f;

    bool stepping_effector = false;
    bool became_grounded = false;
};

}  // namespace AnimConstraints
}  // namespace tmt
TMT_COMPONENT(tmt::AnimConstraints::DampedTransformConstraint, "Damped Transform Constraint", (damp));
TMT_COMPONENT(tmt::AnimConstraints::TwoBoneIKConstraint, "Two Bone IK Constraint", (mid, tip, target_position_entity, bend_position_entity, obey_foot_bind_pose));
TMT_COMPONENT(
    tmt::AnimConstraints::EffectorWalkCycle, "Effector Walk Cycle",
    (step_time, step_duration, step_height, desired_target_entity, effector_entity, ground_entity, cycle_offset, grounded, step_prediction_strength, stepping_curve, height_curve)
);
// TMT_COMPONENT(tmt::AnimConstraints::EffectorWalkCycle, "Effector e"
