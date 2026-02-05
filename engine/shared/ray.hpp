#pragma once

#include "engine\core\entity.hpp"

namespace tmt {

/* Ray definition. */
struct Ray {
    glm::vec3 origin {};
    glm::vec3 dir {};
    glm::vec3 rcp_dir {};

    Ray() = default;
    Ray(glm::vec3 origin, glm::vec3 dir) : origin(origin), dir(dir), rcp_dir(glm::clamp(1.0f / dir, -1e30f, 1e30f)) {};
};

/* Ray hit data. */
struct Hit {
    /* Distance to the object we hit in world-space. */
    float distance = 1e30f;
    /* Entity of the object we hit. */
    Entity entity {entt::null};
    /* Voxel coordinate inside the object we hit. */
    glm::uvec3 coord {};
    /* Normal of the hit in world-space. */
    glm::vec3 normal {};

    Hit() = default;
    Hit(float distance, Entity entity, glm::uvec3 coord, glm::vec3 normal) : distance(distance), entity(entity), coord(coord), normal(normal) {};

    /* @returns True if this hit was a miss. */
    inline bool miss() const { return distance == 1e30f; };

    /* @returns True if this hit was a hit. */
    operator bool() const { return !miss(); };
};

}  // namespace tmt
