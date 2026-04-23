#include "compass_icon.hpp"

#include "engine/core/polyline.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"
void game::CompassIcon::update(const tmt::FrameData& time) {
    // movement related

    if (relative_entity == entt::null) {
        return;
    }
    auto& relative_transform { tmt::engine.ecs.get_component<tmt::Transform>(relative_entity) };

    auto& player_transform { tmt::engine.ecs.get_component<tmt::Transform>(game::Player::get().entity) };
    auto player_forward { player_transform.get_forward() };

    auto relative_vector { glm::normalize(relative_transform.get_world_position() - player_transform.get_world_position()) };
    auto forward_factor { glm::dot(player_forward, relative_vector) };
    auto cross = glm::cross(player_forward, relative_vector);
    float signed_cross { glm::dot(cross, { 0.0, 1.0f, 0.0f }) };
    move_to_position(glm::atan(signed_cross, forward_factor));
    // change image when above or below
    bool below_barge { player_transform.get_world_position().y < relative_transform.get_world_position().y };
    tmt::Entity disable;

    if (below_barge) {
        disable = enable_below;
    } else {
        disable = enable_above;
    }
    if (disable != entt::null) {
        tmt::engine.ecs.disable(disable);
    }
}
void game::CompassIcon::move_to_position(float factor) const {
    tmt::engine.ecs.get_component<tmt::Transform>(entity).set_local_position({ factor * radius, 0.0f, 0.0f });
}