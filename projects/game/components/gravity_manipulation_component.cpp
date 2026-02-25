#include "gravity_manipulation_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "engine/core/polyline.hpp"
#include "game_input.hpp"
#include "player.hpp"

namespace game {

void GravityManipulationComponent::start() {}

void GravityManipulationComponent::update(const tmt::FrameData& time) {
    if (!active) return;

    push_cooldown_counter -= time.delta_time;
    if (tmt::engine.ecs.has_component<game::Player>(entity) || input_active_on_non_player) {
        auto& input = tmt::engine.input;
        if (input.is_action_pressed(action::SHOOT)) {
            // if mb1 down
            grav_point_check();
            grav_attract();
        }
        if (input.is_action_pressed(action::SECONDARY_TOOL_USE)) {
            // if mb2 down
            grav_point_check();
            grav_shoot();
        }
    }
}

void GravityManipulationComponent::end() {}

void GravityManipulationComponent::grav_point_check() {
    if (!active) return;
    currently_manipulated_entities.clear();
    auto physical_entity_view = tmt::engine.ecs.get_registry().view<tmt::VoxelBody>();

    tmt::engine.polyline.draw_sphere(tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position(), range);

    for (entt::entity entity_vb_view : physical_entity_view) {
        tmt::VoxelBody& current_vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(entity_vb_view);
        // First filter for max mass
        if (current_vb.get_mass() < max_mass) {
            float distance = glm::distance(current_vb.position, tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position());
            // Filter for distance
            if (distance < range) {
                currently_manipulated_entities.push_back(entity_vb_view);
            }
        }
    }
}

void GravityManipulationComponent::grav_attract() {
    if (!active) return;
    for (auto manip_entity : currently_manipulated_entities) {
        auto& curr_vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(manip_entity);

        // Distance and direction calc
        float distance = glm::length(tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position() - curr_vb.position);
        glm::vec3 direction = glm::normalize(tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position() - curr_vb.position);

        // Distance factor to slow down when near attract point
        float dist_factor = glm::clamp(distance/range, 0.0f, 1.0f);

        // Calculate target velocity
        glm::vec3 target_velocity = direction * pull_strength * dist_factor;

        // Lerp towards the target velocity
        curr_vb.velocity = glm::mix(curr_vb.velocity, target_velocity, attraction_acceleration);
    }
}

void GravityManipulationComponent::grav_shoot() {
    if (!active) return;
    if (push_cooldown_counter > 0) return;
    // Calculate player looking direction
    glm::vec3 direction = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_forward();

    for (auto manip_entity : currently_manipulated_entities) {
        auto& curr_vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(manip_entity);
        curr_vb.velocity += direction * push_strength;
    }

    // Set cooldown
    push_cooldown_counter = push_cooldown;
}

}  // namespace game
