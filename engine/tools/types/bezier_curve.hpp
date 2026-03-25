#pragma once

#include "engine/core/reflection.hpp"

namespace tmt {

struct BezierCurve {
    // Linear by default
    std::vector<float> values { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f };

    float eval(float x) const {
        float t = x;  // initial guess
        for (int i = 0; i < 5; i++) {
            const float dx = bezier_dx(t);
            if (abs(dx) < 1e-6) break;
            t -= (bezier_x(t) - x) / dx;
            t = glm::clamp(t, 0.f, 1.f);
        }
        return glm::clamp(bezier_y(t), 0.f, 1.f);
    }

    inline glm::vec4 get_vec4() const { return glm::vec4(values[0], values[1], values[2], values[3]); }

   private:
    // Bezier Helpers
    float bezier_x(float t) const {
        const float u = 1.0f - t;
        return 3.0f * u * u * t * values[0] + 3.0f * u * t * t * values[2] + t * t * t;
    }
    float bezier_y(float t) const {
        const float u = 1.0f - t;
        return 3.0f * u * u * t * values[1] + 3.0f * u * t * t * values[3] + t * t * t;
    }
    float bezier_dx(float t) const {
        const float u = 1.0f - t;
        return 3.0f * u * u * values[0] + 6.0f * u * t * (values[2] - values[0]) + 3.0f * t * t * (1.0f - values[2]);
    }
};

}  // namespace tmt

TMT_OBJECT(tmt::BezierCurve, (values));
