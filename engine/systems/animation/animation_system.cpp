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
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"

namespace tmt {

Entity try_find_rig_entity(Entity start) {
    auto& transform = engine.ecs.get_component<Transform>(start);
    for (auto parent : transform.get_all_parents()) {
        auto rig_model_comp = engine.ecs.try_get_component<RigModel>(parent);
        if (rig_model_comp) {
            return parent;
        }
    }
    return entt::null;
}

void RigModelManager::on_start() {
    for (const auto&& [entity, rig] : engine.ecs.view<RigModel>().each()) {
        for (const Entity bone_entity : rig.bone_entities) {
            if (!engine.ecs.valid(bone_entity)) continue;  // Check if the bone entity is actually valid (we'd prefer the list is guaranteed to be accurate, but this works fine.).

            auto& transform = engine.ecs.get_component<Transform>(bone_entity);

            rig.bone_keyframes[bone_entity] = { .translation = transform.get_local_position(), .rotation = transform.get_local_rotation(), .scale = transform.get_local_scale() };
        }
    }

    // for now check for constraints and add constrained rig
    for (const auto&& [entity, transform, two_bone_constraint] : engine.ecs.view<Transform, AnimConstraints::TwoBoneIKConstraint>().each()) {
        Entity rig_ent = try_find_rig_entity(entity);

        if (rig_ent != entt::null) {
            two_bone_constraint.constrained_rig_ent = rig_ent;
        }
    }
}
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

                                                           // auto& transform = engine.ecs.get_component<Transform>(bone_entity);

            const BoneComp& bone_comp_id = engine.ecs.get_component<BoneComp>(bone_entity);
            Bone& bone = rig.data->bones[bone_comp_id.id];

            auto& local_keyframe = rig.bone_keyframes[bone_entity];

            rig.animate_translation(local_keyframe, bone);
            rig.animate_rotation(local_keyframe, bone);
            rig.animate_scale(local_keyframe, bone);
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

    engine.polyline.use_color(0.0f, 1.0f, 0.0f);
    engine.polyline.use_line_width(0.75f);

    for (const auto&& [entity, transform, rig] : engine.ecs.view<Transform, RigModel>().each()) {
        for (const Entity bone_entity : rig.bone_entities) {
            if (!engine.ecs.valid(bone_entity)) continue;

            auto& bone_transform = engine.ecs.get_component<Transform>(bone_entity);
            const glm::vec3 joint = bone_transform.get_world_position();

            if (bone_transform.has_parent()) {
                auto& parent_transform = engine.ecs.get_component<Transform>(bone_transform.get_parent());
                const glm::vec3 parent_joint = parent_transform.get_world_position();
                engine.polyline.draw_line(parent_joint, joint);
            }
        }
    }
}

void tmt::AnimationConstraintSystem::on_start() {
    for (const auto&& [rig_ent, rig_model] : engine.ecs.view<RigModel>().each()) {
        auto& constrained_rig = engine.ecs.add_component<ConstrainedRig>(rig_ent);
        constrained_rig.initial_reference_poses = rig_model.bone_keyframes;
    }

    for (const auto&& [entity, transform, damped_constraint] : engine.ecs.view<Transform, AnimConstraints::DampedTransformConstraint>().each()) {
        damped_constraint.local_rest_pose.bone_entity = entity;
        AnimConstraints::DampedTransform::set_local_rest_pose(transform, damped_constraint.local_rest_pose);
    }
    for (const auto&& [entity, transform, two_bone_constraint] : engine.ecs.view<Transform, AnimConstraints::TwoBoneIKConstraint>().each()) {
        two_bone_constraint.root = entity;
        two_bone_constraint.parent = transform.get_parent();

        auto* root_transform = engine.ecs.try_get_component<Transform>(two_bone_constraint.root);
        auto* mid_transform = engine.ecs.try_get_component<Transform>(two_bone_constraint.mid);
        auto* end_transform = engine.ecs.try_get_component<Transform>(two_bone_constraint.tip);

        if (!root_transform || !mid_transform || !end_transform) {
            tmt::Log::error("[AnimationConstraintSystem] Please set all entities in the chain to prevent unwanted behaviour");
        } else {
            auto& constrained_rig = engine.ecs.get_component<ConstrainedRig>(two_bone_constraint.constrained_rig_ent);
            auto& rig = engine.ecs.get_component<RigModel>(two_bone_constraint.constrained_rig_ent);

            constrained_rig.constrained_poses[two_bone_constraint.root] = rig.bone_keyframes[two_bone_constraint.root];
            constrained_rig.constrained_poses[two_bone_constraint.mid] = rig.bone_keyframes[two_bone_constraint.mid];
            constrained_rig.constrained_poses[two_bone_constraint.tip] = rig.bone_keyframes[two_bone_constraint.tip];

            glm::vec3 root_axis = glm::normalize(mid_transform->get_local_position());
            glm::vec3 mid_axis = glm::normalize(end_transform->get_local_position());

            glm::quat solved_swing;
            AnimConstraints::TwoBoneIK::decompose_swing_twist(root_transform->get_local_rotation(), root_axis, solved_swing, two_bone_constraint.twist_rest_pose_root);
            AnimConstraints::TwoBoneIK::decompose_swing_twist(mid_transform->get_local_rotation(), root_axis, solved_swing, two_bone_constraint.twist_rest_pose_mid);

            if (two_bone_constraint.parent != entt::null) {
                constrained_rig.constrained_poses[two_bone_constraint.parent] = rig.bone_keyframes[two_bone_constraint.parent];

                auto& parent_transform = engine.ecs.get_component<Transform>(two_bone_constraint.parent);
                two_bone_constraint.foot_parent_rest_rotation = glm::inverse(parent_transform.get_world_rotation()) * end_transform->get_world_rotation();
            }
        }
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

        input_data.root_orig_twist = two_bone_constraint.twist_rest_pose_root;
        input_data.mid_orig_twist = two_bone_constraint.twist_rest_pose_mid;
        input_data.foot_parent_reference = two_bone_constraint.foot_parent_rest_rotation;

        auto output = AnimConstraints::TwoBoneIK::solve_two_bone_ik(input_data);

        if (two_bone_constraint.constrained_rig_ent != entt::null) {
            auto& constrained_rig = engine.ecs.get_component<ConstrainedRig>(two_bone_constraint.constrained_rig_ent);
            auto& rig_model = engine.ecs.get_component<RigModel>(two_bone_constraint.constrained_rig_ent);

            constrained_rig.constrained_poses[two_bone_constraint.parent] = rig_model.bone_keyframes[two_bone_constraint.parent];

            auto& local_keyframe_root = constrained_rig.constrained_poses[two_bone_constraint.root];
            auto& local_keyframe_mid = constrained_rig.constrained_poses[two_bone_constraint.mid];
            auto& local_keyframe_tip = constrained_rig.constrained_poses[two_bone_constraint.tip];

            local_keyframe_root.rotation = output.root_rot;
            local_keyframe_mid.rotation = output.mid_rot;
            if (two_bone_constraint.obey_foot_bind_pose) local_keyframe_tip.rotation = output.end_rot;
        } else {
            // just set it directly
            root_transform.set_local_rotation(output.root_rot);
            mid_transform.set_local_rotation(output.mid_rot);
            if (two_bone_constraint.obey_foot_bind_pose) tip_transform.set_local_rotation(output.end_rot);
        }
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
void AnimationPoseEvaluator::on_update(const tmt::FrameData&) {
    for (const auto&& [entity, rig] : engine.ecs.view<RigModel>().each()) {
        for (const Entity bone_entity : rig.bone_entities) {
            auto& transform = engine.ecs.get_component<Transform>(bone_entity);
            const auto& local_pose = rig.bone_keyframes[bone_entity];
            if (auto voxel_body = tmt::engine.ecs.try_get_component<tmt::VoxelBody>(bone_entity)) {
                voxel_body->position = local_pose.translation;
                voxel_body->rotation = local_pose.rotation;
            } else {
                transform.set_local_position(local_pose.translation);
                transform.set_local_rotation(local_pose.rotation);
                transform.set_local_scale(local_pose.scale);
            }
        }
    }

    for (const auto&& [entity, rig, constrained_rig] : engine.ecs.view<RigModel, ConstrainedRig>().each()) {
        for (const auto& [constr_ent, pose] : constrained_rig.constrained_poses) {
            auto& transform = engine.ecs.get_component<Transform>(constr_ent);

            const auto& local_keyframe_pose = rig.bone_keyframes[constr_ent];
            const auto& local_constrained_pose = constrained_rig.constrained_poses[constr_ent];

            glm::vec3 blend_pose_position = glm::mix(local_keyframe_pose.translation, local_constrained_pose.translation, constrained_rig.blend);
            glm::quat blend_pose_rotation = glm::slerp(local_keyframe_pose.rotation, local_constrained_pose.rotation, constrained_rig.blend);
            glm::vec3 blend_pose_scale = glm::mix(local_keyframe_pose.scale, local_constrained_pose.scale, constrained_rig.blend);

            if (auto voxel_body = tmt::engine.ecs.try_get_component<tmt::VoxelBody>(entity)) {
                voxel_body->position = blend_pose_position;
                voxel_body->rotation = blend_pose_rotation;
            } else {
                transform.set_local_position(blend_pose_position);
                transform.set_local_rotation(blend_pose_rotation);
                transform.set_local_scale(blend_pose_scale);
            }
        }
    }
}

}  // namespace tmt
