#pragma once

#include "engine/systems/animation/rig_model.hpp"
#include "engine/tools/serializer.hpp"

/* Deserialize only */
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::RigModel& value) {
    JsonReflect::Detail::from_json_visitable(j, value);

    if (value.data == nullptr) return;

    const auto entity = tmt::engine.ecs.get_entity(value);
    value.armature_entity = entity;

    // Reload to get the animations from the separate animation files.
    value.data->reload();

    if (!value.data) return;

    const tmt::Transform& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    const std::set<tmt::Entity>& children = transform.get_all_children();

    for (const tmt::Entity child : children) {
        const tmt::BoneComp* bone = tmt::engine.ecs.try_get_component<tmt::BoneComp>(child);

        if (bone == nullptr) continue;

        value.bone_entities.push_back(child);
    }
}