#include "two_bone_ik.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"
#include "engine/core/logger.hpp"
#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"

namespace tmt {
namespace AnimConstraints {
namespace TwoBoneIK {

TwoBoneIKSolverOutput solve_two_bone_ik(const TwoBoneInputData& input_data) {
    // world space solver input state variables
    glm::vec3 in_root, in_mid, in_end;
    glm::quat in_parent_rot, in_root_rot, in_mid_rot;

    // transform the data to the same coordinate space (world)
    if (input_data.parent) {
        in_parent_rot = glm::quat_cast(glm::orthonormalize(glm::mat3(input_data.parent->get_world_matrix())));
        in_parent_rot = glm::normalize(in_parent_rot);

        in_root = input_data.parent->get_world_matrix() * glm::vec4(input_data.root->get_local_position(), 1.f);
        in_root_rot = in_parent_rot * input_data.root->get_local_rotation();
        in_root_rot = glm::normalize(in_root_rot);
    } else {
        // assume root is a world transform

        // identity
        in_parent_rot = glm::quat(1.f, 0.f, 0.f, 0.f);

        in_root = input_data.root->get_local_position();
        in_root_rot = input_data.root->get_local_rotation();
    }

    in_mid = input_data.root->get_world_matrix() * glm::vec4(input_data.mid->get_local_position(), 1.f);
    in_mid_rot = in_root_rot * input_data.mid->get_local_rotation();
    in_mid_rot = glm::normalize(in_mid_rot);

    in_end = input_data.mid->get_world_matrix() * glm::vec4(input_data.end->get_local_position(), 1.f);

    // analytical (exact) solution for two bone ik using law of cosines and trig
    float upperarm_length = glm::distance(in_root, in_mid);
    float forearm_length = glm::distance(in_mid, in_end);

    glm::vec3 to_end = in_end - in_root;
    glm::vec3 to_end_normalized = glm::normalize(to_end);
    glm::vec3 to_mid = in_mid - in_root;

    glm::vec3 to_effector = input_data.effector - in_root;
    glm::vec3 to_effector_dir = glm::normalize(to_effector);

    // construct the basis vectors for the pole vector based on a bend pos hint
    glm::vec3 to_bend_dir = glm::normalize(input_data.bend_pos - in_root);
    glm::vec3 ortho = glm::normalize(glm::cross(to_end_normalized, to_bend_dir));

    glm::vec3 pole = glm::cross(ortho, to_end_normalized);
    float to_effector_dist = glm::length(to_effector);

    // small subtraction since if the bones are completely stretched we can't figure out the plane and the calculations become unstable af

    float subtraction = (upperarm_length + forearm_length) * 0.01f;
    float max_length = upperarm_length + forearm_length - subtraction;

    if (to_effector_dist > max_length) {
        to_effector = to_effector_dir * max_length;
    } else if (to_effector_dist < 0.001f) {
        to_effector = to_effector_dir * 0.001f;
    }

    // compute rotation between current and target dir
    glm::quat between_poles = glm::rotation(to_end_normalized, to_effector_dir);
    glm::vec3 new_pole_dir = between_poles * pole;

    // law of cosines to figure out the angle between to_effector and to_mid: a^2 = b^2 + c^2 - 2 * b * c * cos(alpha)
    // rewritten to compute alpha = acos( (b^2 + c^2 - a^2) / 2bc)
    float to_effector_length = glm::length(to_effector);

    float denom = 2.f * upperarm_length * to_effector_length;
    float cos_angle = 0.f;
    if (denom > FLT_EPSILON) {
        cos_angle = glm::clamp(((upperarm_length * upperarm_length) + (to_effector_length * to_effector_length) - (forearm_length * forearm_length)) / denom, -1.f, 1.f);
    }

    float angle = acosf(cos_angle);

    // write new positions to output, they are in world space though. probably not useful to the receiving caller but good for debug drawing
    TwoBoneIKSolverOutput output;
    output.end = in_root + to_effector;

    // cos_angle can be used as inputs to sin and cos for a 2d vector on the solving plane
    output.mid = in_root + (to_effector_dir * (cos_angle * upperarm_length)) + new_pole_dir * (sinf(angle) * upperarm_length);

    // compute the rotation delta between old and new root bone orientations
    glm::vec3 to_mid_new = output.mid - in_root;
    glm::quat root_delta_rotation = glm::rotation(glm::normalize(to_mid), glm::normalize(to_mid_new));

    // add to current root rotation, and result is in world space so transform it back into local space
    glm::quat root_local_rot = glm::inverse(in_parent_rot) * (root_delta_rotation * in_root_rot);
    output.root_rot = root_local_rot;

    // figure out the delta rotation of the middle bone, its downstream so we do need to take into account the root delta rotation
    // fuck this shit
    glm::vec3 in_end_with_root_rotation = in_root + root_delta_rotation * (in_end - in_root);
    glm::vec3 mid_to_end_old = in_end_with_root_rotation - output.mid;
    glm::vec3 mid_to_end_new = output.end - output.mid;
    glm::quat mid_delta_rotation = glm::rotation(glm::normalize(mid_to_end_old), glm::normalize(mid_to_end_new));

    // add it to the current rotation and transform back into local space
    glm::quat mid_local_rot = glm::inverse(in_root_rot) * (mid_delta_rotation * in_mid_rot);
    output.mid_rot = mid_local_rot;

    output.bend_angle = glm::pi<float>() - glm::angle(glm::rotation(glm::normalize(in_root - output.mid), glm::normalize(output.end - output.mid)));

    return output;
}

glm::vec3 EffectorWalkCycle::do_walk_cycle(float dt, const WalkCycleUpdateVariables& walk_cycle_vars) {
    step_timer += dt;

    glm::vec3 point_velocity = (walk_cycle_vars.continuous_available_pos - last_continuous_pos) / dt;
    last_continuous_pos = walk_cycle_vars.continuous_available_pos;

    if (step_timer >= step_time + cycle_offset && !stepping_effector) {
        step_timer -= step_time;

        stepping_effector = true;

        initial_pos = effector;
    }

    // predict based on positional change
    glm::vec3 prediction_vec = (point_velocity * dt) * step_prediction_strength;
    glm::vec3 subtraction_vec = prediction_vec * walk_cycle_vars.up;
    prediction_vec -= subtraction_vec;

    /* if (glm::length(prediction_vec) > 0.5f) {
        prediction_vec = glm::normalize(prediction_vec) * 0.5f;
    }*/

    desired_pos = walk_cycle_vars.continuous_available_pos + prediction_vec;

    if (walk_cycle_vars.grounded) {
        if (!became_grounded) {
            became_grounded = true;
            initial_pos = walk_cycle_vars.continuous_available_pos + walk_cycle_vars.up * step_height;
            desired_pos = walk_cycle_vars.continuous_available_pos;
            stepping_effector = true;
        }

        if (stepping_effector) {
            float distance_height_factor = glm::clamp(glm::distance(initial_pos, walk_cycle_vars.continuous_available_pos), 0.f, 1.f);

            glm::vec3 mix_step = glm::mix(initial_pos, desired_pos, step_interp);
            float height_step = sinf(glm::pi<float>() * step_interp) * step_height * distance_height_factor * static_cast<float>(walk_cycle_vars.grounded);

            glm::vec3 heightvec = walk_cycle_vars.up * height_step;
            mix_step += heightvec;

            effector = mix_step;

            step_interp += dt * (1.f / step_duration);

            if (step_interp >= 1.f) {
                step_interp = 0.f;

                stepping_effector = false;
            }
        }
    } else {
        effector = glm::mix(effector, walk_cycle_vars.continuous_available_pos + walk_cycle_vars.up * step_height, 20.f * dt);
        became_grounded = false;
    }
    return effector;
}

void EffectorWalkCycle::set_offset(float offset) {
    cycle_offset = offset;
}

}  // namespace TwoBoneIK
}  // namespace AnimConstraints
}  // namespace tmt
