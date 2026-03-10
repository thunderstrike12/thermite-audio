#pragma once
#include "glm/fwd.hpp"

namespace tmt {

struct Transform;

namespace AnimConstraints {

struct LocalRestPose;
namespace DampedTransform {

void set_local_rest_pose(const tmt::Transform& transform, LocalRestPose& rest_pose);
void update_damped_pose(LocalRestPose& rest_pose, const glm::vec3& parent_world_pos, const glm::quat& parent_world_rot, float damp);

}  // namespace DampedTransform

}  // namespace AnimConstraints
}  // namespace tmt