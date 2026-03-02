#pragma once
#include "engine/core/reflection.hpp"
#include "engine/tools/types/color.hpp"
namespace game {

struct DebugLineConfig {
    tmt::RGBA color { glm::vec4 { 1.0f } };
    float line_width = 1.0f;
    void set_values() const;
};

}  // namespace game

TMT_OBJECT(game::DebugLineConfig, (color, line_width));
