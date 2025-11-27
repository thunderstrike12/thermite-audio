#pragma once

namespace tmt {

/* Axis Aligned Bounding Box. */
struct Aabb {
    glm::vec3 min = glm::vec3(BIG_F32);
    glm::vec3 max = glm::vec3(-BIG_F32);
};

}  // namespace tmt
