#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <glm/gtc/quaternion.hpp>


namespace tmt::TestMotion { // aka lotta vibecode
struct Params {
    glm::vec3 center {0.0f};
    glm::vec3 amp {10.5f, 5.6f, 10.5f};  // overall range per-axis
    float speed = 1.0f;                // global speed scalar

    // Secondary wobble (adds "life" without noise)
    glm::vec3 wobble_amp {0.25f, 0.12f, 0.25f};
    glm::vec3 wobble_freq {2.7f, 3.9f, 3.3f};

    // Slow drift of the center (prevents repeating in-place feel)
    float drift_amp = 0.8f;
    glm::vec2 drift_freq {0.12f, 0.08f};  // x/z drift frequencies
};

inline glm::vec3 sample_position(float t_seconds, const Params& p = {}) {
    const float t = t_seconds * p.speed;

    // Figure-eight-ish base in XZ + vertical bob in Y
    const float x0 = p.amp.x * std::sin(1.0f * t);
    const float z0 = p.amp.z * std::sin(1.0f * t) * std::cos(1.0f * t);  // 0.5*sin(2t) shape
    const float y0 = p.amp.y * (0.65f * std::sin(2.2f * t) + 0.35f * std::sin(0.7f * t + 1.1f));

    // Secondary wobble (different frequencies per axis)
    const float x1 = p.wobble_amp.x * std::sin(p.wobble_freq.x * t + 0.3f);
    const float y1 = p.wobble_amp.y * std::sin(p.wobble_freq.y * t + 2.1f);
    const float z1 = p.wobble_amp.z * std::sin(p.wobble_freq.z * t + 1.7f);

    // Slow drifting center
    const glm::vec3 drift(p.drift_amp * std::sin(p.drift_freq.x * t), 0.0f, p.drift_amp * std::cos(p.drift_freq.y * t));

    return p.center + drift + glm::vec3(x0 + x1, y0 + y1, z0 + z1);
}

inline glm::quat sample_rotation(float t_seconds, float dt, const Params& p = {}) {
    // Finite difference velocity
    glm::vec3 p0 = sample_position(t_seconds, p);
    glm::vec3 p1 = sample_position(t_seconds + dt, p);
    glm::vec3 v = (p1 - p0) / dt;

    if (glm::dot(v, v) < 1e-8f) v = glm::vec3(0, 0, 1);

    return glm::quat(glm::radians(glm::vec3(0.f, -90.f, 0.f))) * glm::quatLookAtLH(glm::normalize(v), glm::vec3(0, 1, 0));
}
}  // namespace tmt::TestMotion