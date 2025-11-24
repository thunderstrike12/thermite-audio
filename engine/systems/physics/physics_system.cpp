#include "physics_system.hpp"
#include <iostream>

namespace tmt {

void PhysicsSystem::init() {
    std::cout << "[Thermite][Physics] Initialized.\n";
}

void PhysicsSystem::update(float dt) {
    std::cout << "[Thermite][Physics] Updating with dt = " << dt
              << " seconds.\n";
}

}  // namespace tmt
