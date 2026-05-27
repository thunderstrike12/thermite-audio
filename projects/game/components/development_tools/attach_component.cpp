#include "attach_component.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/input/input.hpp"
#include "glm/gtx/norm.inl"
#include "projects/game/components/gameplay_functionality_components/enable_movement_tips.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"
#include "projects/game/components/ui_components/movement_tip.hpp"
#include "projects/game/data_headers/game_input.hpp"
#include "projects/game/components/gameplay_functionality_components/mover_component.hpp"
namespace {

void set_attached_entity_transform(tmt::Entity entity, tmt::Entity parent = entt::null) {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    transform.set_parent(parent);
    if (parent != entt::null) {
        transform.set_local_position(glm::vec3(0.0f));
    }
}

}  // namespace
void game::AttachComponent::start() {
    // By default assign the player entity
    if (entity_that_attaches == entt::null) {
        auto player_view = tmt::engine.ecs.view<game::Player>();
        for (auto& element : player_view) {
            entity_that_attaches = element.entity;
            break;
        }
    }
    tmt::engine.ecs.get_dispatcher().sink<AttachAttemptEvent>().connect<&AttachComponent::on_check_range_to_attach>(this);

    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& mover_comp = tmt::engine.ecs.get_component<MoverComponent>(transform.get_parent());

    tmt::engine.ecs.get_dispatcher().sink<TriggerMovementStopEvent>().connect<&MoverComponent::stop_movement>(mover_comp);
}
void game::AttachComponent::update(const tmt::FrameData& time) {
    if (is_attached == false) {
        if (is_moving) tmt::engine.ecs.get_dispatcher().trigger(TriggerMovementStopEvent {});

        is_moving = false;
        has_started_pressing = false;

        if (entity_that_attaches != entt::null) {
            // triggers when we are in range to attach, will display press E to attach
            tmt::engine.ecs.get_dispatcher().trigger(InRangeEvent { .entity = entity, .in_range = check_inside_range({ entity_that_attaches }) });
        }
        // enable movement tip if possible on the first exit from the barge
        if (once) {
            once = false;
            auto view { tmt::engine.ecs.view<EnableMovementTips>(entt::exclude_t {}) };
            if (view.empty() == false) {
                std::get<0>(view.front().components).enable_entities_in_order();
            }
        }
        return;
    }

    auto& input = tmt::engine.input;
    if (input.is_action_just_pressed(action::TRIGGER_BARGE_MOVEMENT)) {
        has_started_pressing = true;
    }
    if (input.is_action_just_released(action::TRIGGER_BARGE_MOVEMENT)) {
        has_started_pressing = false;
    }

    // if we have been pressing for a while trigger it, if we keep pressing after the fact ignore
    if (first_frame || (has_started_pressing == true && input.get_action_duration(action::TRIGGER_BARGE_MOVEMENT) > time_to_start_stop_barge_movement)) {
        is_moving = !is_moving;
        first_frame = false;
        has_started_pressing = false;
        tmt::Log::info("Movement is {}", is_moving);

        if (!is_moving) {
            tmt::engine.ecs.get_dispatcher().trigger(TriggerMovementStopEvent {});
        }
    }

    if (is_moving) {
        tmt::engine.ecs.get_dispatcher().trigger<TriggerMovementEvent>({ .trigger = entity });
    }

    auto player { Player::get().entity };
    auto& player_component { tmt::engine.ecs.get_component<Player>(player) };
    // ending run logic
    if (player_component.player_ended_run == false && input.get_action_duration(action::TRIGGER_RUN_END) > time_to_end_run) {
        player_component.player_ended_run = true;
        tmt::engine.ecs.get_dispatcher().trigger<EndRun>({ .player_dead = false });
    }
}
void game::AttachComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<AttachAttemptEvent>().disconnect<&AttachComponent::on_check_range_to_attach>(this);
}
void game::AttachComponent::draw_debug_lines() const {
    cfg.set_values();

    if (entity_that_attaches != entt::null && check_inside_range({ entity_that_attaches })) {
        tmt::engine.polyline.use_color(glm::vec4 { 1.0f, 0.0f, 0.0f, 1.0f });
    }
    tmt::engine.polyline.draw_sphere(tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position(), radius);
}
bool game::AttachComponent::check_inside_range(const AttachAttemptEvent& event) const {
    auto entity_pos = tmt::engine.ecs.get_component<tmt::Transform>(event.entity).get_world_position();
    const auto pos = tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position();

    return glm::distance2(entity_pos, pos) < radius * radius;
}
void game::AttachComponent::on_check_range_to_attach(const AttachAttemptEvent& event) {
    if (event.entity == entt::null || tmt::engine.ecs.valid(event.entity) == false) {
        return;
    }
    // TODO this only works if the entity that is attached to this has no parent.

    if (is_attached) {
        is_attached = false;
        set_attached_entity_transform(entity_that_attaches, entt::null);
        tmt::engine.ecs.get_dispatcher().trigger<AttachEvent>({ .entity = entity_that_attaches, .is_attached = is_attached });

        return;
    }
    // attach if is inside and the flag is active
    if (check_inside_range(event)) {
        is_attached = true;
        tmt::Log::info("Entity {} is attached to {}", entity_that_attaches, entity);
        // TODO maybe do not base it on the parent, should be fine for now though
        // TODO add a mover component to this
        set_attached_entity_transform(entity_that_attaches, entity);

        tmt::engine.ecs.get_dispatcher().trigger<AttachEvent>({ .entity = entity_that_attaches, .is_attached = is_attached });
    }
}
