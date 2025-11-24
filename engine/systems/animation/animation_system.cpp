#include "animation_system.hpp"
#include <iostream>

namespace tmt {

void AnimationSystem::init() {
    std::cout << "[Thermite][Animation] Initialized.\n";
}

void AnimationSystem::update(float dt) {
    std::cout << "[Thermite][Animation] Updating with dt = " << dt
              << " seconds.\n";
}

}  // namespace tmt
