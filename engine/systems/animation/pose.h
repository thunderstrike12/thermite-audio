#pragma once
#include "glm/fwd.hpp"

namespace tmt {

struct Pose {
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

}  // namespace tmt
