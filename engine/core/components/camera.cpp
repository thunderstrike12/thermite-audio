#include "camera.hpp"

#include "engine.hpp"
#include "transform.hpp"

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

}  // namespace tmt