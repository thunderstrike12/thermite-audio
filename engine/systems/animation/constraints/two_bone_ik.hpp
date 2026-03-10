#pragma once

#include "glm/fwd.hpp"
#include "engine/core/components/transform.hpp"

namespace tmt {
namespace AnimConstraints {
namespace TwoBoneIK {

// bone output data
struct TwoBoneIKSolverOutput {
    glm::vec3 mid;
    glm::vec3 end;
    glm::quat root_rot;
    glm::quat mid_rot;

    // might be useful to get muscle strength or sum
    float bend_angle;
};

struct TwoBoneInputData {
    // bones
    // can be left nullptr
    tmt::Transform* parent = nullptr;

    tmt::Transform* root = nullptr;
    tmt::Transform* mid = nullptr;
    tmt::Transform* end = nullptr;

    // target
    glm::vec3 effector;

    // the bend position helper
    glm::vec3 bend_pos;
};

TwoBoneIKSolverOutput solve_two_bone_ik(const TwoBoneInputData& input_data);

// helper struct to be used for a walk cycle motion for an effector position
// usage is create once and call DoWalkCycle continuously
struct EffectorWalkCycle {
    // step after how much time?
    float step_time;

    // step motion duration
    float step_duration;

    // step motion height
    float step_height;

    // placement prediction strength, uses the difference in position to predict next placement
    float step_prediction_strength = 1.f;

    // variables used for updating state in the walk cycle
    struct WalkCycleUpdateVariables {
        glm::vec3 continuous_available_pos;
        bool grounded;
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
    };

    // returns the output effector
    //  continuous_available_pos: retrieve by means of raycast for example
    glm::vec3 do_walk_cycle(float dt, const WalkCycleUpdateVariables& walk_cycle_variables);

    // adds offset to timer variable to offset step interval
    void set_offset(float offset);

    inline bool is_moving() const { return stepping_effector; }

   private:
    float step_timer = 0.f;
    float step_interp = 0.f;
    float cycle_offset = 0.f;

    bool stepping_effector = false;
    bool became_grounded = false;

    glm::vec3 last_continuous_pos;
    glm::vec3 initial_pos;
    glm::vec3 desired_pos;
    // the internally tracked effector
    glm::vec3 effector;
};

}  // namespace TwoBoneIK
}  // namespace AnimConstraints
}  // namespace tmt