#pragma once
#include "collision.hpp"

namespace tmt {

constexpr float RESTITUTION = 0.1f;  // Global bounciness coefficient
constexpr float FRICTION = 0.3f;     // Global friction coefficient
constexpr size_t MAX_CONTACTS = 10000;

class ConstraintSolver {
   public:
    ConstraintSolver() { collisions.resize(MAX_CONTACTS); };

    void solve_velocities(const float time_step);
    void solve_positions(const float time_step);

    std::atomic<size_t> contact_index = 0;
    std::vector<Collision> collisions = {};
    int velocity_iterations = 16;
    int position_iterations = 4;
};

}  // namespace tmt
