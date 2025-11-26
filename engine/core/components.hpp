#pragma once
#include <string>
#include <glm/glm.hpp>

#include "engine/tools/fmt_helpers.hpp"

namespace tmt {
struct Name {
    std::string name;
};

struct Transform {
    glm::vec3 position {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation {0.0f, 0.0f, 0.0f};
    glm::vec3 scale {1.0f, 1.0f, 1.0f};
};

}  // namespace tmt

FMT_LOGGING(tmt::Name, "name: \"{}\"", obj.name);
FMT_LOGGING(tmt::Transform, "Position: {}, Rotation: {}, Scale: {}", obj.position, obj.rotation, obj.scale);