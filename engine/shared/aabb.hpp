#pragma once

#include "const.hpp"

namespace tmt {

/* Axis Aligned Bounding Box. */
struct Aabb {
    glm::vec3 min = glm::vec3(BIG_F32);
    glm::vec3 max = glm::vec3(-BIG_F32);

    Aabb() = default;
    Aabb(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}

    bool overlap(const Aabb& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) && (min.y <= other.max.y && max.y >= other.min.y) && (min.z <= other.max.z && max.z >= other.min.z);
    }
};

}  // namespace tmt
