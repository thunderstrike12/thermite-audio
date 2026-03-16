#include "fuel.hpp"
#include "mover_component.hpp"
namespace {

game::MoverComponent& get_mover_component(tmt::Entity entity) {
    return tmt::engine.ecs.get_component<game::MoverComponent>(entity);
}

}  // namespace
void game::FuelComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<MovementUpdateEvent>().connect<&FuelComponent::on_trigger_movement>(this);

    if (current_fuel > 0.0f) {
        tmt::engine.ecs.get_dispatcher().sink<TriggerMovementEvent>().connect<&MoverComponent::update_movement>(get_mover_component(mover_entity));
    }
}
void game::FuelComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<MovementUpdateEvent>().disconnect<&FuelComponent::on_trigger_movement>(this);

    tmt::engine.ecs.get_dispatcher().sink<TriggerMovementEvent>().disconnect<&MoverComponent::update_movement>(get_mover_component(mover_entity));
}

void game::FuelComponent::on_trigger_movement(const MovementUpdateEvent& event) {
    if (event.moved_entity != mover_entity) {
        return;
    }

    current_fuel -= event.length * decrease_multiplier;
    if (current_fuel <= 0.0f) {
        current_fuel = 0.0f;
        tmt::Log::info("No more fuel");
        tmt::engine.ecs.get_dispatcher().sink<TriggerMovementEvent>().disconnect<&MoverComponent::update_movement>(get_mover_component(mover_entity));
    }
}
