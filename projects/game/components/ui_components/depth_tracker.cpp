#include "depth_tracker.hpp"

#include "engine/core/components/text_renderer.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"

namespace game {

void DepthTracker::start() {
    // Default get player one if not set
    if (player_entity == entt::null) {
        player_entity = Player::get().entity;
    }
}

void DepthTracker::update(const tmt::FrameData& time) {
    measure_distance();
    change_text();
}

void DepthTracker::measure_distance() {
    tmt::Transform* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    tmt::Transform* barge_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(barge_entity);
    if (!barge_transform) {
        tmt::Log::error("Unable to get transform component of barge entity {} on depth tracker component. Check entity {} depth tracker.", barge_entity, entity);
        return;
    }
    if (!player_transform) {
        tmt::Log::error("Unable to get transform component of player entity {} on depth tracker component. Check entity {} depth tracker.", player_entity, entity);
        return;
    }
    glm::vec3 player_pos = player_transform->get_world_position();
    glm::vec3 barge_pos = barge_transform->get_world_position();
    glm::vec3 barge_direction = glm::normalize(barge_transform->get_forward());

    if (distance_type == DepthTrackerDistanceType::TOTAL_DISTANCE) {
        distance = glm::distance(player_pos, barge_pos);
    } else if (distance_type == DepthTrackerDistanceType::PATH_DISTANCE) {
        // Vector from barge to player
        glm::vec3 barge_to_player = player_pos - barge_pos;
        // Project onto the barge's forward direction
        float t = glm::dot(barge_to_player, barge_direction);
        // Closest point on the line
        glm::vec3 closest_point = barge_pos + t * barge_direction;

        distance = glm::distance(player_pos, closest_point);
    }
}

void DepthTracker::change_text() const {
    tmt::TextRenderer* text_renderer_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(entity);
    if (!text_renderer_component) {
        tmt::Log::error("Unable to change text on depth tracker component due to missing text renderer component. Check entity {}", entity);
        return;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(displayed_decimals) << distance;
    text_renderer_component->text = ss.str();
}

}  // namespace game