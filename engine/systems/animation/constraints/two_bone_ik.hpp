#pragma once

#include "glm/fwd.hpp"
#include "engine/core/components/transform.hpp"

#include "engine/systems/animation/pose.h"

namespace tmt {
namespace AnimConstraints {
namespace TwoBoneIK {

// bone output data
struct TwoBoneIKSolverOutput {
    glm::vec3 mid;
    glm::vec3 end;
    glm::quat root_rot;
    glm::quat mid_rot;
    glm::quat end_rot;

    // might be useful to get muscle strength or sum
    float bend_angle;
};

struct TwoBoneInputData {
    // bones
    tmt::Transform* parent = nullptr;

    tmt::Transform* root = nullptr;
    tmt::Transform* mid = nullptr;
    tmt::Transform* end = nullptr;

    glm::quat root_orig_twist;
    glm::quat mid_orig_twist;
    glm::quat foot_parent_reference;

    // target
    glm::vec3 effector;

    // the bend position helper
    glm::vec3 bend_pos;
};

TwoBoneIKSolverOutput solve_two_bone_ik(const TwoBoneInputData& input_data);
// AI given
void decompose_swing_twist(const glm::quat& rotation, const glm::vec3& twist_axis, glm::quat& out_swing, glm::quat& out_twist);

}  // namespace TwoBoneIK
}  // namespace AnimConstraints
}  // namespace tmt
