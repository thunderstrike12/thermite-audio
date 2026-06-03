#include "gravity_manipulation_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/polyline.hpp"
#include "weapon.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/data_headers/save_entries.hpp"

#include "engine/core/input/input.hpp"
#include "projects/game/data_headers/game_input.hpp"

#include "../player.hpp"

namespace game {

void GravityManipulationComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&GravityManipulationComponent::on_weapon_fired>(this);
    tmt::engine.ecs.get_dispatcher().sink<ReleaseShootEvent>().connect<&GravityManipulationComponent::on_release>(this);

    max_mass = tmt::engine.player_data.get<float>(GRAVITY_GUN_DATA, max_mass);
}

void GravityManipulationComponent::update(const tmt::FrameData& time) {}

void GravityManipulationComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&GravityManipulationComponent::on_weapon_fired>(this);
    tmt::engine.ecs.get_dispatcher().sink<ReleaseShootEvent>().disconnect<&GravityManipulationComponent::on_release>(this);
}
void GravityManipulationComponent::draw_debug_lines() const {
    if (attraction_point_entity == entt::null) return;
    auto attraction_pos = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position();

    tmt::engine.polyline.draw_sphere(attraction_pos, range);
}

void GravityManipulationComponent::on_weapon_fired(const WeaponFiredEvent& e) {
    if (e.weapon_entity != entity) return;

    grav_point_check();
    if (e.secondary_shot == false) {
        grav_attract();

        if (sounds.gravity_activate.is_valid() && !gravity_active_instance.is_valid()) gravity_active_instance = sounds.gravity_activate.play();

    } else if (!currently_manipulated_entities.empty()) {
        if (sounds.gravity_launch_object.is_valid()) sounds.gravity_launch_object.play();
        grav_shoot();

        // Get the animated rig for the tool animations
        auto* rig_controller = tmt::engine.ecs.try_get_component<tmt::RigController>(animated_tool_entity);
        if (rig_controller) {
            rig_controller->set_parameter_trigger("Shoot");
        }
    }
}

void GravityManipulationComponent::on_release(const ReleaseShootEvent& e) {}

void GravityManipulationComponent::grav_point_check() {
    const auto start_size = currently_manipulated_entities.size();

    currently_manipulated_entities.clear();
    auto attraction_pos = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position();

    auto physical_entity_view = tmt::engine.ecs.view<tmt::VoxelBody>();
    for (entt::entity e : physical_entity_view) {
        auto& vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(e);
        if (vb.type == tmt::VoxelBody::STATIC) continue;
        if (vb.get_mass() >= max_mass) continue;

        float distance = glm::distance(vb.center_of_mass, attraction_pos);
        if (distance < range) {
            currently_manipulated_entities.push_back(e);
        }
    }

    if (sounds.gravity_hold_object.is_valid() && currently_manipulated_entities.size() > start_size) sounds.gravity_hold_object.play();
}

void GravityManipulationComponent::grav_attract() {
    auto attraction_pos = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position();

    for (auto e : currently_manipulated_entities) {
        auto& vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(e);
        float distance = glm::length(attraction_pos - vb.center_of_mass);
        glm::vec3 direction = glm::normalize(attraction_pos - vb.center_of_mass);
        float dist_factor = glm::clamp(distance / range, 0.0f, 1.0f);
        glm::vec3 target_velocity = direction * pull_strength * dist_factor;
        vb.velocity = glm::mix(vb.velocity, target_velocity, attraction_acceleration);
    }
}

void GravityManipulationComponent::grav_shoot() {
    glm::vec3 direction = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_forward();
    for (auto e : currently_manipulated_entities) {
        auto& vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(e);
        vb.velocity += direction * push_strength;
    }
}

}  // namespace game
