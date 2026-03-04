#include "rig_renderer.hpp"
#include "engine/systems/animation/components/bone_hierarchy_renderer.hpp"
#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/ecs.hpp"
#include "glm/gtx/quaternion.hpp"

void tmt::RenderBoneHierarchy(const tmt::Transform& root) 
{
    const auto& children = root.get_children();

    if(children.empty()) return;

    glm::vec3 origin = root.get_world_position();

    for (auto child : children) {

        if(tmt::engine.ecs.has_component<tmt::NoBone>(child)) continue;

        auto& child_transform = tmt::engine.ecs.get_component<Transform>(child);
        glm::vec3 child_origin = child_transform.get_world_position();

        glm::vec3 dir = child_origin - origin;
        float length = glm::length(dir);

        glm::quat rot = glm::rotation(glm::vec3(0.f, 1.f, 0.f), glm::normalize(dir));

        tmt::engine.polyline.draw_bone(origin, rot, length);

        RenderBoneHierarchy(child_transform);
    }
}
