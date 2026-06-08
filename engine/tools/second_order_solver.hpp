#pragma once
#include "glm/gtc/constants.hpp"
#include <concepts>

namespace tmt {

template <typename T>
concept ValidTarget = requires(T a, T b, float f) {
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { a * f } -> std::convertible_to<T>;
};

class SecondOrderSolver {
   public:
    // requires the operators -, +, * with float
    template <typename T>
    requires ValidTarget<T>
    struct State {
        T current_target, current_state, current_velocity;
    };

    template <typename T>
    static void solve(State<T>& state, const T& target, float f, float z, float r, float delta_time) {
        const float k1 = z / (glm::pi<float>() * f);
        float k2 = 1.f / ((2.f * glm::pi<float>() * f) * (2.f * glm::pi<float>() * f));
        const float k3 = (r * z) / (2.f * glm::pi<float>() * f);

        float k2_stable = std::max(k2, 1.1f * (delta_time * delta_time / 4.f + delta_time * k1 / 2.f));

        T estimated_velocity = (target - state.current_target) / delta_time;
        state.current_target = target;

        state.current_state = state.current_state + delta_time * state.current_velocity;
        state.current_velocity = state.current_velocity + delta_time * (target + k3 * estimated_velocity - state.current_state - k1 * state.current_velocity) / k2_stable;
    }
};

}  // namespace tmt