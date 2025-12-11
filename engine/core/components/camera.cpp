#include "camera.hpp"

#include "engine.hpp"
#include "transform.hpp"
#include "engine/core/ecs.hpp"

namespace tmt {

Entity Camera::get_active_camera() {
    /* Capture all cameras in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const Camera>(entt::get<Transform>);

    for (auto&& [entity, camera, transform] : group.each()) {
        if (camera.active == true) {
            return entity;
        }
    }

    return entt::null;
}

void Camera::set_active_camera(Entity camera_entity) {
    /* Capture all cameras in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<Camera>(entt::get<Transform>);

    for (auto&& [entity, camera, transform] : group.each()) {
        if (entity == camera_entity) {
            camera.active = true;
        } else {
            camera.active = false;
        }
    }
}

}  // namespace tmt