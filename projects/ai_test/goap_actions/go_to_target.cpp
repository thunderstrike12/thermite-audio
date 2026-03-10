#include "go_to_target.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"

namespace tmt {

void GoToTarget::on_start(Entity agent) {
    auto& registry = engine.ecs.get_registry();

    // Create or reset steering request
    SteeringRequest request {};
    request.mode = SteeringMode::ARRIVE;
    request.target_position = target;

    registry.emplace_or_replace<SteeringRequest>(agent, request);
}

bool GoToTarget::is_done(Entity agent) const {
    auto& registry = engine.ecs.get_registry();
    if (!registry.any_of<SteeringRequest>(agent)) return true;

    auto& request = registry.get<SteeringRequest>(agent);
    return request.completed;
}

void GoToTarget::on_finished(Entity agent) {
    auto& registry = engine.ecs.get_registry();
    if (registry.any_of<SteeringRequest>(agent)) {
        registry.remove<SteeringRequest>(agent);
    }
}

void GoToTarget::on_interrupt(Entity agent) {
    on_finished(agent);  // clean up
}

}  // namespace tmt
