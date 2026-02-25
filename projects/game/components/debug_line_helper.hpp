#pragma once
#include "glm/vec4.hpp"
#include "engine/core/reflection.hpp"
namespace game {

struct DebugLineConfig {
    glm::vec4 color { 1.0f };
    float line_width = 1.0f;
    void set_values() const;
};

}  // namespace game

TMT_OBJECT(game::DebugLineConfig, (color, line_width));
