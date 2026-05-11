#include "compass_icon.hpp"
#include "engine/core/polyline.hpp"
#include "glm/ext/matrix_common.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"

#include <extern/type_tween/type_tween.hpp>

void game::CompassIcon::follow_relative_transform(const glm::vec3 relative_vector) {
    const auto& player_transform { tmt::engine.ecs.get_component<tmt::Transform>(game::Player::get().entity) };
    const auto player_forward { player_transform.get_forward() };

    const auto to_direction { glm::normalize(relative_vector - player_transform.get_world_position()) };
    const auto forward_factor { glm::dot(player_forward, to_direction) };
    const auto cross { glm::cross(player_forward, to_direction) };
    const float signed_cross { glm::dot(cross, { 0.0, 1.0f, 0.0f }) };
    move_to_position(glm::atan(signed_cross, forward_factor));
    // change image when above or below
    const bool below_barge { relative_vector.y < player_transform.get_world_position().y };
    tmt::Entity disable;

    if (enable_below != entt::null) {
        tmt::engine.ecs.enable(enable_below);
    }
    if (enable_above != entt::null) {
        tmt::engine.ecs.enable(enable_above);
    }

    if (below_barge) {
        disable = enable_below;
    } else {
        disable = enable_above;
    }
    if (disable != entt::null) {
        tmt::engine.ecs.disable(disable);
    }
}
void game::CompassIcon::follow_world_direction(glm::vec3 world_dir) {
    const auto& player_transform { tmt::engine.ecs.get_component<tmt::Transform>(game::Player::get().entity) };
    const auto player_forward { player_transform.get_forward() };

    const auto to_direction { glm::normalize(world_dir) };
    const auto forward_factor { glm::dot(player_forward, to_direction) };
    const auto cross { glm::cross(player_forward, to_direction) };
    const float signed_cross { glm::dot(cross, glm::vec3 { 0.0f, 1.0f, 0.0f }) };

    move_to_position(glm::atan(signed_cross, forward_factor));
}
void game::CompassIcon::update(const tmt::FrameData&) {
    // movement related, if we have a relative target we use it, otherwise default to the user provided value
    if (relative_entity != entt::null) {
        follow_relative_transform(tmt::engine.ecs.get_component<tmt::Transform>(relative_entity).get_world_position());
    } else {
        follow_world_direction(glm::vec3 { default_placement.x, 0.0f, default_placement.y });
    }
}
void game::CompassIcon::move_to_position(float factor) {
    // between 0 and 1
    auto close_to_edge { glm::abs(factor) / max_threshold };
    // get the alpha

    tmt::engine.ecs.get_dispatcher().trigger(IconTransitionEvent { entity, close_to_edge, min_threshold });

    auto increment { glm::round(factor / move_increments) * move_increments };
    tmt::engine.ecs.get_component<tmt::Transform>(entity).set_local_position({ increment * radius, 0.0f, 0.0f });
}