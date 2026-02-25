#include "animation_system.hpp"

#include "core/logger.hpp"

#include "engine.hpp"
#include "rig_model.hpp"
#include "animation_data.hpp"
#include "engine/core/polyline.hpp"

#define CONDITION(comparison) [](const Variant& parameter, const Variant& value) -> bool { return parameter comparison value; }

namespace tmt {

std::string tmt::RigModelManager::get_name() {
    return "animation system";
}

void tmt::RigModelManager::on_game_start() {
    for (const auto& [entity, rig] : engine.ecs.view<RigModel>().each()) {
        auto& rigmodel = tmt::engine.ecs.get_component<tmt::RigModel>(entity);
        rigmodel.init(rigmodel.data.file_location, entity);
    }
}

void tmt::RigModelManager::on_start() {}

void RigModelManager::on_update(const FrameData& time) {
    inspect(time.delta_time);
    for (const auto& [entity, rig] : engine.ecs.view<RigModel>().each()) {
        auto path = rig.vox_path.relative_path;
        if (!rig.vox_is_loaded && !path.empty()) {
            rig.attach_voxel_objects();
        }

        if (rig.data == nullptr) continue;

        if (rig.state != RigModel::State::STATIONARY && !rig.is_transferring()) rig.time += rig.animation_speed * time.delta_time;

        if (rig.is_transferring()) rig.transfer_time += time.delta_time * rig.animation_speed;

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
                    break;

                case RigModel::State::ANIMATE_LOOP:
                case RigModel::State::ANIMATE_ONCE:
                case RigModel::State::STATIONARY:
                    break;
            }

            rig.time = 0.0f;
            rig.set_current_animation(rig.get_next_animation());
            rig.transfer_time = 0.0f;
        }

        const Bone& last_bone = rig.data->bones.back();
        const std::string& current_animation = rig.get_current_animation();
        if (last_bone.animations.find(current_animation) == last_bone.animations.end()) {
            Log::warn(R"(Animation "{}" isn't a valid animation!)", current_animation);
            continue;
        }

        const float animation_time = last_bone.animations.at(rig.get_current_animation()).keyframes_pos.back().time;

        if (rig.time >= animation_time) {
            if (rig.state == RigModel::State::ANIMATE_ONCE) {
                rig.state = RigModel::State::STATIONARY;
            } else if (rig.state == RigModel::State::ANIMATE_LOOP) {
                rig.time = rig.time - animation_time;
            }
        }

        for (const Entity bone_entity : rig.bone_entities) {
            auto& transform = engine.ecs.get_component<Transform>(bone_entity);

            const BoneComp& bone_comp_id = engine.ecs.get_component<BoneComp>(bone_entity);
            Bone& bone = rig.data->bones[bone_comp_id.id];

            rig.animate_translation(transform, bone);
            rig.animate_rotation(transform, bone);
            rig.animate_scale(transform, bone);
        }
    }
}

void tmt::RigModelManager::on_end() {}

void RigModelManager::inspect(float) {
    int lines_drawn = 0;
    engine.polyline.use_color(1.0f, 0.3f, 0.3f);
    engine.polyline.use_line_width(0.25f);
    for (const auto& [entity, transform, rig] : engine.ecs.view<Transform, RigModel>().each()) {
        for (const Entity bone_entity : rig.bone_entities) {
            auto& bone_transform = engine.ecs.get_component<Transform>(bone_entity);
            // auto& bone_name = engine.ecs.get_component<Name>(bone_entity);
            const glm::vec3 joint = bone_transform.get_world_position();

            if (bone_transform.has_parent()) {
                auto& parent_transform = engine.ecs.get_component<Transform>(bone_transform.get_parent());
                const glm::vec3 parent_joint = parent_transform.get_world_position();
                engine.polyline.draw_line(parent_joint, joint);
                lines_drawn++;
            }
        }
    }
}

}  // namespace tmt
