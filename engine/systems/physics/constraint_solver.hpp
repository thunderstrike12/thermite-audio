#pragma once
#include "collision.hpp"

namespace tmt {
class ConstraintSolver {
   public:
    ConstraintSolver() { collisions.reserve(64); };

    void solve_velocities(const float time_step);
    void solve_positions(const float time_step);

    std::vector<Collision> collisions = {};
    int velocity_iterations = 16;
    int position_iterations = 8;
};
}  // namespace tmt
