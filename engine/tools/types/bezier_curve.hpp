#pragma once

#include "engine/core/reflection.hpp"

namespace tmt {

struct BezierCurve {
    // Linear by default
    std::vector<float> values { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f };

    inline glm::vec4 get_vec4() const { return glm::vec4(values[0], values[1], values[2], values[3]); }
};

}  // namespace tmt

TMT_OBJECT(tmt::BezierCurve, (values));