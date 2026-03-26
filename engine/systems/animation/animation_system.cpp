#include "animation_system.hpp"

#include "core/logger.hpp"

#include "engine.hpp"
#include "rig_model.hpp"
#include "rig_renderer.hpp"
#include "animation_data.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/animation/components/bone_hierarchy_renderer.hpp"
#include "engine/systems/animation/constraints/damped_transform.hpp"
#include "engine/systems/animation/constraints/two_bone_ik.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/rig_controller.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"

namespace tmt {

void RigModelManager::on_update(const FrameData& time) {
    for (const auto&& [entity, rig, controller] : engine.ecs.view<RigModel, RigController>().each()) {
        const std::string& current_state = controller.current_state;
        if (current_state.empty()) {
            const auto& name = engine.ecs.get_component<Name>(entity);
            Log::error("Trying to animate \"{}\" without a state!", name);
            continue;
        }

        for (auto& transition : controller.transitions) {
            if (transition.from != current_state) continue;

            std::vector<Trigger*> satisfied_triggers;
            const bool should_transition = controller.check_transition_conditions(transition, satisfied_triggers);
            if (!should_transition) continue;

            const AnimationState& from_state = controller.states[transition.from];
            if (from_state.transition_on_finish) {
                const auto& animations = rig.data->bones.back().animations;
                const auto& iterator = animations.find(from_state.animation);

                if (iterator != animations.end() && rig.time < (iterator->second.keyframes_pos.back().time - transition.transition_time)) continue;
            }

            // If the transition is successful we reset all the trigger parameters
            std::ranges::for_each(satisfied_triggers, [](Trigger* trigger) { *trigger = Trigger { false }; });

            const std::string& state_name = controller.current_state = transition.to;
            const AnimationState& state = controller.states[state_name];

            rig.set_animation_speed(state.animation_speed);

            if (state.animation.empty()) {
                if (rig.state == RigModel::State::ANIMATE_LOOP) rig.stop_loop(transition.transition_time);
                break;
            }

            rig.play_animation(state.animation, transition.transition_time, state.repeat);
            break;
        }
    }

    for (const auto&& [entity, rig] : engine.ecs.view<RigModel>().each()) {
        if (rig.data == nullptr || rig.state == RigModel::State::STATIONARY) continue;

        if (rig.is_transferring()) {
            rig.transfer_time += time.delta_time * rig.animation_speed;

            if (rig.transfer_time > rig.transfer_threshold) {
                switch (rig.state) {
                    case RigModel::State::TRANSFERRING_TO_LOOP:
                        rig.state = RigModel::State::ANIMATE_LOOP;
                        break;
                    case RigModel::State::TRANSFERRING_TO_ONCE:
                        rig.state = RigModel::State::ANIMATE_ONCE;
                        break;
                    case RigModel::State::TRANSFERRING_TO_STOP:
                        rig.state = RigModel::State::STATIONARY;
                        continue;  // This continue is for the current for loop, rather than the break which would be for the switch statement.

                    default:
                        break;
                }

                rig.time = 0.0f;
                rig.set_current_animation(rig.get_next_animation());
                rig.transfer_time = 0.0f;
            }
        } else {
            rig.time += rig.animation_speed * time.delta_time;
        }

        const Bone& last_bone = rig.data->bones.back();
        const std::string& current_animation = rig.get_current_animation();
        if (!last_bone.animations.contains(current_animation)) {
            Log::warn(R"(Animation "{}" isn't a valid animation!)", current_animation);
            continue;
        }

        const float animation_time = last_bone.animations.at(current_animation).keyframes_pos.back().time;

        if (rig.time >= animation_time) {
            if (rig.state == RigModel::State::ANIMATE_ONCE) {
                rig.state = RigModel::State::STATIONARY;
                continue;  // When state is stationary we should skip the rest of the animation process.
            }

            if (rig.state == RigModel::State::ANIMATE_LOOP) {
                rig.time = rig.time - animation_time;
            }
        }

        // Checks for when the amount of bone entities does not match the amount of bones the rig has.
        if (rig.bone_entities.empty())
            Log::error(Log::Scope::ENGINE, "Tried to animate RigModel without any bone entities!");
        else if (rig.bone_entities.size() < rig.data->bones.size())
            Log::warn(Log::Scope::ENGINE, "Tried to animate RigModel with some missing bone entities!");

        for (const Entity bone_entity : rig.bone_entities) {
            if (!engine.ecs.valid(bone_entity)) continue;  // Check if the bone entity is actually valid (we'd prefer the list is guaranteed to be accurate, but this works fine.).

            auto& transform = engine.ecs.get_component<Transform>(bone_entity);

            const BoneComp& bone_comp_id = engine.ecs.get_component<BoneComp>(bone_entity);
            Bone& bone = rig.data->bones[bone_comp_id.id];

            rig.animate_translation(transform, bone);
            rig.animate_rotation(transform, bone);
            rig.animate_scale(transform, bone);
        }
    }
}

void RigModelManager::on_draw_lines() const {
    for (const auto&& [entity, transform, bone_renderer] : engine.ecs.view<Transform, BoneHierarchyRenderer>().each()) {
        tmt::engine.polyline.use_depth_testing(true);
        tmt::engine.polyline.use_color(bone_renderer.color);
        tmt::engine.polyline.use_line_width(bone_renderer.line_width);
        tmt::RenderBoneHierarchy(transform);
    }

    for (const auto&& [entity, transform, bend_hint] : engine.ecs.view<Transform, BendHint>().each()) {
        tmt::engine.polyline.use_color(glm::vec4(1.f, 0.f, 0.f, 1.f));
        tmt::engine.polyline.draw_sphere(transform.get_world_position(), bend_hint.radius);
    }

    for (const auto&& [entity, transform, effector] : engine.ecs.view<Transform, Effector>().each()) {
        tmt::engine.polyline.use_color(glm::vec4(0.f, 0.7f, 0.7f, 1.f));
        tmt::engine.polyline.draw_sphere(transform.get_world_position(), effector.radius);
    }
}

void tmt::AnimationConstraintSystem::on_start() {
    for (const auto&& [entity, transform, damped_constraint] : engine.ecs.view<Transform, AnimConstraints::DampedTransformConstraint>().each()) {
        damped_constraint.local_rest_pose.bone_entity = entity;
        AnimConstraints::DampedTransform::set_local_rest_pose(transform, damped_constraint.local_rest_pose);
    }
    for (const auto&& [entity, transform, two_bone_constraint] : engine.ecs.view<Transform, AnimConstraints::TwoBoneIKConstraint>().each()) {
        two_bone_constraint.root = entity;
        two_bone_constraint.parent = transform.get_parent();
    }
}
void tmt::AnimationConstraintSystem::on_update(const tmt::FrameData& time) {
    for (const auto&& [entity, transform, damped_constraint] : engine.ecs.view<Transform, AnimConstraints::DampedTransformConstraint>().each()) {
        damped_constraint.local_rest_pose.local_pos = transform.get_local_position();
        damped_constraint.local_rest_pose.local_rot = transform.get_local_rotation();

        AnimConstraints::DampedTransform::update_damped_pose(
            damped_constraint.local_rest_pose, damped_constraint.local_rest_pose.local_pos, damped_constraint.local_rest_pose.local_rot, damped_constraint.damp
        );
    }
    for (const auto&& [entity, transform, two_bone_constraint] : engine.ecs.view<Transform, AnimConstraints::TwoBoneIKConstraint>().each()) {
        AnimConstraints::TwoBoneIK::TwoBoneInputData input_data;
        if (engine.ecs.valid(two_bone_constraint.parent)) {
            input_data.parent = &engine.ecs.get_component<Transform>(two_bone_constraint.parent);
        }

        auto& root_transform = transform;
        auto& mid_transform = engine.ecs.get_component<Transform>(two_bone_constraint.mid);
        auto& tip_transform = engine.ecs.get_component<Transform>(two_bone_constraint.tip);

        input_data.root = &root_transform;
        input_data.mid = &mid_transform;
        input_data.end = &tip_transform;

        auto& bend_target_transform = engine.ecs.get_component<Transform>(two_bone_constraint.bend_position_entity);
        auto& target_position_transform = engine.ecs.get_component<Transform>(two_bone_constraint.target_position_entity);

        input_data.effector = target_position_transform.get_world_position();
        input_data.bend_pos = bend_target_transform.get_world_position();

        auto output = AnimConstraints::TwoBoneIK::solve_two_bone_ik(input_data);

        root_transform.set_local_rotation(output.root_rot);
        mid_transform.set_local_rotation(output.mid_rot);
    }
    for (const auto&& [entity, transform, walk_cycle] : engine.ecs.view<Transform, AnimConstraints::EffectorWalkCycle>().each()) {
        walk_cycle.step_timer += time.delta_time;
        auto parent = transform.get_parent();
        auto& parent_transform = engine.ecs.get_component<Transform>(parent);
        auto up = parent_transform.get_up();

        glm::vec3 continuous_available_pos = engine.ecs.get_component<Transform>(walk_cycle.desired_target_entity).get_world_position();
        if (auto nav_mesh = engine.ecs.try_get_component<NavMesh>(walk_cycle.ground_entity)) {
            int idx = nav_mesh->find_closest_node(continuous_available_pos);
            if (idx != -1) up = (*nav_mesh->nodes_mesh)[idx].normal;

            Ray ray;
            ray.dir = -up;
            ray.origin = continuous_available_pos + up * 0.5f;
            Hit hit = engine.renderer.trace_ray(ray);

            if (!hit.miss()) {
                continuous_available_pos = ray.origin + ray.dir * hit.distance;
                walk_cycle.grounded = true;
            } else {
                // continuous_available_pos = ;
                walk_cycle.grounded = false;
            }
        }

        glm::vec3 point_velocity = (continuous_available_pos - walk_cycle.last_continuous_pos) / time.delta_time;
        walk_cycle.last_continuous_pos = continuous_available_pos;

        if (walk_cycle.step_timer >= walk_cycle.step_time + walk_cycle.cycle_offset && !walk_cycle.stepping_effector) {
            walk_cycle.step_timer -= walk_cycle.step_time;

            walk_cycle.stepping_effector = true;

            walk_cycle.initial_pos = walk_cycle.effector;
        }

        // predict based on positional change
        glm::vec3 prediction_vec = (point_velocity * time.delta_time) * walk_cycle.step_prediction_strength;
        glm::vec3 subtraction_vec = prediction_vec * up;
        prediction_vec -= subtraction_vec;

        /* if (glm::length(prediction_vec) > 0.5f) {
            prediction_vec = glm::normalize(prediction_vec) * 0.5f;
        }*/

        walk_cycle.desired_pos = continuous_available_pos + prediction_vec;

        if (walk_cycle.grounded) {
            if (!walk_cycle.became_grounded) {
                walk_cycle.became_grounded = true;
                walk_cycle.initial_pos = continuous_available_pos + up * walk_cycle.step_height;
                walk_cycle.desired_pos = continuous_available_pos;
                walk_cycle.stepping_effector = true;
            }

            if (walk_cycle.stepping_effector) {
                float distance_height_factor = glm::clamp(glm::distance(walk_cycle.initial_pos, continuous_available_pos), 0.f, 1.f);

                float t = walk_cycle.stepping_curve.eval(walk_cycle.step_interp);
                float t_step = walk_cycle.height_curve.eval(walk_cycle.step_interp);

                glm::vec3 mix_step = glm::mix(walk_cycle.initial_pos, walk_cycle.desired_pos, t);
                float height_step = sinf(glm::pi<float>() * t_step) * walk_cycle.step_height * distance_height_factor * static_cast<float>(walk_cycle.grounded);

                glm::vec3 heightvec = up * height_step;
                mix_step += heightvec;

                walk_cycle.effector = mix_step;

                walk_cycle.step_interp += time.delta_time * (1.f / walk_cycle.step_duration);

                if (walk_cycle.step_interp >= 1.f) {
                    walk_cycle.step_interp = 0.f;

                    walk_cycle.stepping_effector = false;
                }
            }
        } else {
            walk_cycle.effector = glm::mix(walk_cycle.effector, continuous_available_pos + up * walk_cycle.step_height, 20.f * time.delta_time);
            walk_cycle.became_grounded = false;
        }

        auto& effector_transform = engine.ecs.get_component<Transform>(walk_cycle.effector_entity);
        effector_transform.set_world_position(walk_cycle.effector);
    }
}

void tmt::AnimationConstraintSystem::on_end() {}

std::string tmt::AnimationConstraintSystem::get_name() {
    return std::string();
}

}  // namespace tmt
