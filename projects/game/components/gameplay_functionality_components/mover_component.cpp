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

void game::MoverComponent::update(const tmt::FrameData& time) {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    auto pos = transform.get_world_position();

    velocity = glm::min(velocity, movement_speed);

    if (stop) {
        velocity -= deceleration * time.delta_time;
        if (velocity <= 0.f) {
            velocity = 0.f;
            stop = false;
        }
    }

    pos += velocity * transform.get_forward() * time.delta_time;

    transform.set_world_position(pos);
}

void game::MoverComponent::update_movement(const TriggerMovementEvent& event) {
    if (event.trigger != trigger_entity) {
        return;
    }

    if (!tmt::engine.ecs.is_enabled(move_flair_entity)) tmt::engine.ecs.enable(move_flair_entity);

    velocity += acceleration * tmt::engine.frame_data().delta_time;
    float distance = velocity * tmt::engine.frame_data().delta_time;

    tmt::engine.ecs.get_dispatcher().trigger<MovementUpdateEvent>({ .moved_entity = entity, .length = distance });
}

void game::MoverComponent::stop_movement(const TriggerMovementStopEvent& event) {
    tmt::Log::info("MoverComponent: movement stopped");
    if (move_flair_entity != entt::null) {
        tmt::engine.ecs.disable(move_flair_entity);
    }
    stop = true;
}
