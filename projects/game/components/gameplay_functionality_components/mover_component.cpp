#include "mover_component.hpp"

#include "engine/core/polyline.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/data_headers/save_entries.hpp"

#include <projects/game/data_headers/events.hpp>

void game::MoverComponent::start() {
    // TODO check if data
    movement_speed = tmt::engine.player_data.get<float>(BARGE_MOVE_DATA, movement_speed);
}
void game::MoverComponent::draw_debug_lines() const {
    cfg.set_values();
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    tmt::engine.polyline.draw_arrow(transform.get_world_position(), transform.get_forward(), movement_speed);
}

void game::MoverComponent::update_movement(const TriggerMovementEvent& event) {
    if (event.trigger != trigger_entity) {
        return;
    }

    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto distance = movement_speed * tmt::engine.frame_data().delta_time;
    auto delta = transform.get_forward() * distance;
    transform.translate(delta);

    tmt::engine.ecs.get_dispatcher().trigger<MovementUpdateEvent>({ .moved_entity = entity, .length = distance });
}
