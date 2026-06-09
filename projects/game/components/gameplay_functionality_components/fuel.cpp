#include "fuel.hpp"
#include "mover_component.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/data_headers/save_entries.hpp"

namespace {

game::MoverComponent& get_mover_component(tmt::Entity entity) {
    return tmt::engine.ecs.get_component<game::MoverComponent>(entity);
}

}  // namespace
void game::FuelComponent::start() {
    // TODO check if data is there
    fuel_data = tmt::engine.player_data.get<FuelData>(FUEL_DATA, fuel_data);

    current_fuel = fuel_data.max_fuel;

    tmt::engine.ecs.get_dispatcher().sink<MovementUpdateEvent>().connect<&FuelComponent::on_trigger_movement>(this);
    tmt::engine.ecs.get_dispatcher().sink<TriggerMovementStopEvent>().connect<&MoverComponent::stop_movement>(get_mover_component(mover_entity));

    if (current_fuel > 0.0f) {
        tmt::engine.ecs.get_dispatcher().sink<TriggerMovementEvent>().connect<&MoverComponent::update_movement>(get_mover_component(mover_entity));
    }
}

void game::FuelComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<MovementUpdateEvent>().disconnect<&FuelComponent::on_trigger_movement>(this);
    tmt::engine.ecs.get_dispatcher().sink<TriggerMovementStopEvent>().disconnect<&MoverComponent::stop_movement>(get_mover_component(mover_entity));

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
        tmt::engine.ecs.get_dispatcher().trigger<TriggerMovementStopEvent>();
    }
    tmt::engine.ecs.get_dispatcher().trigger<BargeFuelChanged>({ .barge_fuel_entity = entity, .new_value = current_fuel ,.max_value = fuel_data.max_fuel});
}