#include "engine.hpp"

#include "systems/physics/physics_system.hpp"
#include "systems/animation/animation_system.hpp"

#include <iostream>

namespace tmt {

Engine::Engine(std::string name) : name(std::move(name)) {}

const std::string& Engine::get_name() const { return name; }

// Example stuff
void Engine::run() {
    std::cout << "[Thermite] Engine \"" << name << "\" starting...\n";

    PhysicsSystem physics {};
    AnimationSystem animation {};

    physics.init();
    animation.init();

    physics.update(0.016f);
    animation.update(0.016f);

    std::cout << "[Thermite] Application \"" << name << "\" shutting down.\n";
}

}  // namespace tmt
