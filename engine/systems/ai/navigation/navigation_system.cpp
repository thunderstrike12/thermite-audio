#include "navigation_system.hpp"
#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/scene_view.hpp"

std::string tmt::NavigationSystem::get_name() {
    return "NavigationSystem";
}

void tmt::NavigationSystem::on_start() {}

void tmt::NavigationSystem::on_update(const FrameData&) {
    tmt::Entity closest_nav_mesh = entt::null;
    tmt::Entity closest_unfinished_nav_mesh = entt::null;
    float closest_nav_mesh_dist = std::numeric_limits<float>::max();
    float closest_unfinished_nav_mesh_dist = std::numeric_limits<float>::max();
    std::optional<glm::vec3> pos_to_check;
    auto pos_to_check_entity = engine.ecs.view<GenerateClosestNavMeshToThis>().front();
    if (pos_to_check_entity != entt::null) {
        pos_to_check = engine.ecs.get_component<Transform>(pos_to_check_entity).get_world_position();
    }
    if (!pos_to_check) {
        tmt::Log ::warn("No entity with GenerateClosestNavMeshToThis component found, nav meshes will not be updated. Add this component to a player for example.");
        return;
    }
    for (const auto& [entity, transform, nav_mesh] : engine.ecs.view<Transform, NavMesh>().each()) {
        float dist = glm::length(transform.get_world_position() - *pos_to_check);

        if (dist < closest_nav_mesh_dist && nav_mesh.generation_state == NavMeshGenerationState::FINISHED) {
            closest_nav_mesh = entity;
            closest_nav_mesh_dist = dist;
        }
        if (nav_mesh.generation_state != NavMeshGenerationState::FINISHED && dist < closest_unfinished_nav_mesh_dist) {
            closest_unfinished_nav_mesh = entity;
            closest_unfinished_nav_mesh_dist = dist;
        }

        if (!nav_mesh.nodes_mesh) continue;
        nav_mesh.inspect();
    }

    if (auto nav_mesh = tmt::engine.ecs.try_get_component<NavMesh>(closest_nav_mesh)) {
            nav_mesh->check_if_should_regenerate();
    }
    if (auto nav_mesh = tmt::engine.ecs.try_get_component<NavMesh>(closest_unfinished_nav_mesh)) {
        auto& bvh = tmt::engine.renderer.scene_view.bvh;

        if (bvh.nodes[0].left_first == 0u && bvh.nodes[0].prim_count == 0u) {
        } else {
            nav_mesh->generate_mesh_over_time();
        }
    }

    /* for (const auto& [entity, transform, nav_mesh] : engine.ecs.view<Transform, NavMesh>().each()) {
        // if (nav_mesh.generation_state == NavMeshGenerationState::UNINITIALISED) {
        //     nav_mesh.generate_mesh_over_time();
        // }
        if (nav_mesh.generation_state == NavMeshGenerationState::FINISHED) {
            nav_mesh.check_if_should_regenerate();
        }
        glm::mat4 world_matrix = transform.get_world_matrix();
        if (nav_mesh.generation_state != NavMeshGenerationState::UNINITIALISED && nav_mesh.generation_state != NavMeshGenerationState::FINISHED) {
            auto& bvh = tmt::engine.renderer.scene_view.bvh;

            if (bvh.nodes[0].left_first == 0u && bvh.nodes[0].prim_count == 0u) continue;

            // for (auto& node : *nav_mesh.generating_nodes) {
            // node.world_pos = glm::vec3(world_matrix * glm::vec4(node.local_pos, 1.0f));
            //}
            nav_mesh.generate_mesh_over_time();
        }
        if (!nav_mesh.nodes_mesh) continue;

        // for (auto& node : *nav_mesh.nodes_mesh) {
        // node.world_pos = glm::vec3(world_matrix * glm::vec4(node.local_pos, 1.0f));
        //}

        nav_mesh.inspect();
    }*/
}

void tmt::NavigationSystem::on_end() {}
