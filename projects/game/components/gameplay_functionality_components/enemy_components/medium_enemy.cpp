#include "medium_enemy.hpp"
#include "../player.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"

#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "engine/systems/physics/physics_system.hpp"

#include "engine\core\components\voxel_renderer.hpp"

void game::MediumEnemy::start() {
    player = Player::get().entity;

    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    float closest_dist = std::numeric_limits<float>::max();
    for (const auto&& [nav_mesh_entity, nav_mesh] : tmt::engine.ecs.view<tmt::NavMesh>().each()) {
        if (walkable_asteroid == entt::null) walkable_asteroid = nav_mesh_entity;
        auto& nav_mesh_transform = tmt::engine.ecs.get_component<tmt::Transform>(nav_mesh_entity);
        auto& current_walkable_transform = tmt::engine.ecs.get_component<tmt::Transform>(walkable_asteroid);

        float dist = glm::length(nav_mesh_transform.get_world_position() - transform.get_world_position());
        float current_dist = glm::length(current_walkable_transform.get_world_position() - transform.get_world_position());
        if (dist < closest_dist) {
            walkable_asteroid = nav_mesh_entity;
        }
    }
    if (walkable_asteroid == entt::null) {
        tmt::Log::error("No walkable asteroid found for medium enemy! Medium enemy removed.");
        tmt::engine.ecs.destroy_entity(entity);
    } else {
        auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(walkable_asteroid);
        nav_mesh.generate_mesh_over_time();
    }

    // Set ore manager
    auto ore_manager_view = tmt::engine.ecs.view<OreManager>();
    if (!ore_manager_view.empty()) {
        ore_manager = tmt::engine.ecs.try_get_component<OreManager>(ore_manager_view.front().entity);
    } else {
        tmt::Log::warn("No ore manager found in scene, missiles wont explode");
    }
    auto& dispatcher = tmt::engine.ecs.get_dispatcher();
    dispatcher.sink<game::GamePausedEvent>().connect<&MediumEnemy::on_game_paused>(this);
    dispatcher.sink<game::GameUnpausedEvent>().connect<&MediumEnemy::on_game_unpaused>(this);

    // set voxel amounts for core, laser and missile
    {
        auto resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(core).resource;
        core_voxels = resource->blas->voxel_count - resource->blas->voxels_wasted;
    }

    for (const auto& laser_voxel_entity : laser_voxel_entities) {
        auto resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(laser_voxel_entity).resource;
        laser_voxels += resource->blas->voxel_count - resource->blas->voxels_wasted;
    }

    for (const auto& missile_voxel_entity : missile_voxel_entities) {
        auto resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(missile_voxel_entity).resource;
        missile_voxels += resource->blas->voxel_count - resource->blas->voxels_wasted;
    }

    tmt::engine.ecs.get_component<tmt::RigController>(rig_controller).set_parameter_bool("walk", true);
    tmt::engine.ecs.get_component<tmt::ConstrainedRig>(rig_controller).blend = 1.0f;

    auto pos = tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position();
    for (int i = 0; i < 4; i++) {
        if (available_positions[i].entity == entt::null) continue;
        if (available_positions[i].reference_entity == entt::null) continue;
        auto ref_pos = tmt::engine.ecs.get_component<tmt::Transform>(available_positions[i].reference_entity).get_world_position();
        available_positions[i].base_offset_from_ref_entity = ref_pos - pos;
        available_positions[i].stored_offset_from_ref_entity = available_positions[i].base_offset_from_ref_entity;
    }
}

void game::MediumEnemy::update(const tmt::FrameData& time) {
    if (paused || core_destroyed) return;

    tmt::engine.polyline.use_color(1.0f, 0.0f, 0.0f);
    tmt::engine.polyline.use_line_width(2.0f);
    tmt::engine.polyline.use_depth_testing(false);
    tmt::Transform& walking_transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    tmt::engine.polyline.draw_sphere(walking_transform.get_world_position(), 0.5f, 16);

    // world state update
    auto* ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(entity);
    if (!ws) return;
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();
    float dist = glm::length(player_pos - walking_transform.get_world_position());

    // check for core destroyed
    {
        auto resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(core).resource;
        uint32_t current_core_voxels = resource->blas->voxel_count - resource->blas->voxels_wasted;
        if (current_core_voxels != core_voxels) {
            core_destroyed = true;
            tmt::engine.ecs.get_component<tmt::RigController>(rig_controller).set_parameter_bool("walk", false);
            ws->set_fact(tmt::FactId("m_laser_intact"), false);
            ws->set_fact(tmt::FactId("m_missiles_intact"), false);
            auto children = walking_transform.get_all_children();
            for (const auto& child : children) {
                if (!tmt::engine.ecs.try_get_component<tmt::VoxelBody>(child)) continue;
                auto& child_transform = tmt::engine.ecs.get_component<tmt::Transform>(child);
                child_transform.set_parent(entt::null);
                auto& child_voxel_body = tmt::engine.ecs.get_component<tmt::VoxelBody>(child);
                child_voxel_body.velocity = glm::normalize(child_transform.get_world_position() - walking_transform.get_world_position()) * velocity_of_objects_on_death;
                child_voxel_body.type = tmt::VoxelBody::DYNAMIC;
                child_voxel_body.gravity = 0.0f;
                child_voxel_body.position = child_transform.get_world_position();
                child_voxel_body.rotation = child_transform.get_world_rotation();
                auto& renderer = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(child);
                tmt::engine.ecs.systems.try_get<tmt::Physics>()->recalculate_physics_data(child_voxel_body, *renderer.resource.resource);
            }
            return;
        }
    }

    // check for laser destroyed
    int amount_of_laser_voxels_remaining = 0;
    for (const auto& laser_voxel_entity : laser_voxel_entities) {
        auto resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(laser_voxel_entity).resource;
        uint32_t current_laser_voxels = resource->blas->voxel_count - resource->blas->voxels_wasted;
        amount_of_laser_voxels_remaining += current_laser_voxels;
    }
    if (laser_voxels - amount_of_laser_voxels_remaining > laser_voxel_to_lose) {
        ws->set_fact(tmt::FactId("m_laser_intact"), false);
    }

    // check for missile destroyed
    int amount_of_missile_voxels_remaining = 0;
    for (const auto& missile_voxel_entity : missile_voxel_entities) {
        auto resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(missile_voxel_entity).resource;
        uint32_t current_missile_voxels = resource->blas->voxel_count - resource->blas->voxels_wasted;
        amount_of_missile_voxels_remaining += current_missile_voxels;
    }
    if (missile_voxels - amount_of_missile_voxels_remaining > missile_voxel_to_lose) {
        ws->set_fact(tmt::FactId("m_missiles_intact"), false);
    }

    // ranges
    if (dist > aggro_range) {
        ws->set_fact(tmt::FactId("m_in_aggro_range"), false);
    } else {
        ws->set_fact(tmt::FactId("m_in_aggro_range"), true);
    }

    if (dist > laser_range) {
        ws->set_fact(tmt::FactId("m_in_laser_range"), false);
    } else {
        ws->set_fact(tmt::FactId("m_in_laser_range"), true);
    }

    if (dist > stomp_range) {
        ws->set_fact(tmt::FactId("m_in_stomp_range"), false);
    } else {
        ws->set_fact(tmt::FactId("m_in_stomp_range"), true);
    }

    // cooldowns
    missile_timer += time.delta_time;
    if (missile_timer > missile_cooldown) {
        ws->set_fact(tmt::FactId("m_missiles_ready"), true);
    } else {
        ws->set_fact(tmt::FactId("m_missiles_ready"), false);
    }

    laser_timer += time.delta_time;
    if (laser_timer > laser_cooldown) {
        ws->set_fact(tmt::FactId("m_laser_ready"), true);
    } else {
        ws->set_fact(tmt::FactId("m_laser_ready"), false);
    }

    stomp_timer += time.delta_time;
    if (stomp_timer > stomp_cooldown) {
        ws->set_fact(tmt::FactId("m_stomp_ready"), true);
    } else {
        ws->set_fact(tmt::FactId("m_stomp_ready"), false);
    }

    // height correction
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(walkable_asteroid);
    if (!nav_mesh.nav_nodes_valid()) return;

    int closest_node = nav_mesh.find_closest_node(walking_transform.get_world_position());

    auto* nodes = nav_mesh.nodes_mesh;
    auto& normal = (*nodes)[closest_node].normal;

    const tmt::Ray ray = tmt::Ray(walking_transform.get_world_position() + normal * 0.5f, glm::normalize(-normal));
    const tmt::Hit hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray, enemy_mask);

    glm::vec3 move_to = ray.origin + ray.dir * hit.distance - ray.dir * (height_above_ground + height_above_ground_offset);
    glm::vec3 desired_velocity = glm::normalize(move_to - walking_transform.get_world_position()) * walk_speed * 0.5f;
    if (glm::isnan(desired_velocity.x)) {
        tmt::Log::warn("desired vel is nan");
    } else {
        velocity += desired_velocity;
    }

    // movement
    velocity *= 0.9f;
    walking_transform.set_world_position(walking_transform.get_world_position() + velocity * time.delta_time);

    // rotation based off of the ground normal and velocity direction
    glm::vec3 ground_up = glm::normalize(normal);
    glm::quat rot_velocity = glm::quat(1, 0, 0, 0);
    if (glm::dot(velocity, velocity) > 0.00001f) {
        // project velocity onto tangent plane of the ground
        glm::vec3 forward = velocity - ground_up * glm::dot(velocity, ground_up);

        if (glm::dot(forward, forward) > 0.00001f) {
            forward = glm::normalize(forward);
            glm::vec3 right = glm::normalize(glm::cross(ground_up, forward));
            glm::vec3 up = glm::cross(forward, right);

            glm::mat3 basis(right, up, forward);
            rot_velocity = glm::normalize(glm::quat_cast(basis));
        }
    }

    if (glm::dot(velocity, velocity) > 0.5f) {
        tmt::engine.ecs.get_component<tmt::RigController>(rig_controller).set_parameter_bool("walk", true);
        float t = time.delta_time * rotation_speed;
        rotation = glm::slerp(rotation, rot_velocity, glm::clamp(t, 0.0f, 1.0f));
        rotation = glm::normalize(rotation);
        walking_transform.set_world_rotation(rotation);
    } else {
        tmt::engine.ecs.get_component<tmt::RigController>(rig_controller).set_parameter_bool("walk", false);
    }

    // if distance to closest node is too high, teleport to it and set rotation
    // this is for when an enemy is spawned in, it is not in on start becuase the navmesh isn't fully generated yet
    // float distance_to_node = glm::distance(walking_transform.get_world_position(), (*nav_mesh.nodes_mesh)[closest_node].world_pos);
    // if (distance_to_node > 5.0f) {
    //    walking_transform.set_world_position((*nav_mesh.nodes_mesh)[closest_node].world_pos);
    //    walking_transform.set_world_rotation(glm::normalize(rot_velocity));
    //}

    // leg available pos update
    for (int i = 0; i < 4; i++) {
        if (available_positions[i].entity == entt::null) continue;
        if (available_positions[i].reference_entity == entt::null) continue;
        auto& pos_transform = tmt::engine.ecs.get_component<tmt::Transform>(available_positions[i].entity);
        auto world_matrix = walking_transform.get_world_matrix();
        auto ref_pos = glm::vec3(world_matrix * glm::vec4(available_positions[i].stored_offset_from_ref_entity, 1.0f));
        tmt::Ray ray = tmt::Ray();
        // normal = (*nodes)[nav_mesh.find_closest_node(pos_transform.get_world_position())].normal;
        ray.dir = -normal;
        ray.dir = glm::normalize(ray.dir);
        ray.origin = ref_pos + normal * 1.0f;
        tmt::engine.polyline.draw_arrow(ray.origin, ray.dir, 1.0f);
        tmt::Hit hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray, enemy_mask);
        float ground_distance = hit.distance - 1.0f;
        float move_distance = ground_distance - available_positions[i].height_offset > 10.0f ? 10.0f : ground_distance - available_positions[i].height_offset;
        pos_transform.set_world_position(ref_pos + ray.dir * move_distance);
        tmt::engine.polyline.draw_sphere(pos_transform.get_world_position(), 0.1f, 8);
        tmt::engine.polyline.draw_sphere(ref_pos, 0.2f, 8);
    }
}

void game::MediumEnemy::end() {}

void game::MediumEnemy::kite_player() const {
    auto& enemy = tmt::engine.ecs.get_component<MediumEnemy>(entity);
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(enemy.walkable_asteroid);
    if (!nav_mesh.nav_nodes_valid()) return;

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();
    float distance = glm::distance(enemy_entity_pos, player_pos);

    if (std::optional<glm::vec3> direction = nav_mesh.follow_path(enemy_entity_pos, player_pos)) {
        if (distance < back_off_distance) {
            glm::vec3 target_pos = enemy_entity_pos - *direction * 10.0f;
            direction = nav_mesh.follow_path(enemy_entity_pos, target_pos);
            enemy.velocity += glm::vec3(*direction * enemy.walk_speed);
        } else {
            enemy.velocity += glm::vec3(*direction * enemy.walk_speed);
        }
    } else {
        // Close enough to consider node reached, force path recompute
        nav_mesh.path.clear();
    }
}

void game::MediumEnemy::die() {
    // remove GOAP so it doesn't keep acting
    /*auto& registry = tmt::engine.ecs.get_registry();
    auto& agent = registry.get<tmt::GoapAgent>(this);
    tmt::engine.ecs.remove_component<tmt::GoapAgent>(this);*/
}

void game::MediumEnemy::set_stored_offsets_to_ref_entity() {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    glm::mat4 inv_world = glm::inverse(transform.get_world_matrix());
    for (int i = 0; i < 4; i++) {
        if (available_positions[i].entity == entt::null) continue;
        if (available_positions[i].reference_entity == entt::null) continue;
        auto ref_world_pos = tmt::engine.ecs.get_component<tmt::Transform>(available_positions[i].reference_entity).get_world_position();
        // Store as local-space position so world_matrix * offset in update() is correct
        available_positions[i].stored_offset_from_ref_entity = glm::vec3(inv_world * glm::vec4(ref_world_pos, 1.0f));
    }
}

void game::MediumEnemy::set_stored_offsets_to_base() {
    for (int i = 0; i < 4; i++) {
        if (available_positions[i].entity == entt::null) continue;
        if (available_positions[i].reference_entity == entt::null) continue;
        available_positions[i].stored_offset_from_ref_entity = available_positions[i].base_offset_from_ref_entity;
    }
}

void game::MediumEnemy::on_game_paused(const game::GamePausedEvent&) {
    paused = true;
}

void game::MediumEnemy::on_game_unpaused(const game::GameUnpausedEvent&) {
    paused = false;
}
