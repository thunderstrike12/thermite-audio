#include "navigation_system.hpp"
#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/scene_view.hpp"

std::string tmt::NavigationSystem::get_name() {
    return "NavigationSystem";
}

void tmt::NavigationSystem::on_start() {}

void tmt::NavigationSystem::on_update(const FrameData&) {
    for (const auto& [entity, transform, nav_mesh] : engine.ecs.view<Transform, NavMesh>().each()) {
        if (nav_mesh.generation_state == NavMeshGenerationState::UNINITIALISED) {
            nav_mesh.generate_mesh_over_time();
        }
        glm::mat4 world_matrix = transform.get_world_matrix();
        if (nav_mesh.generation_state != NavMeshGenerationState::UNINITIALISED && nav_mesh.generation_state != NavMeshGenerationState::FINISHED) {
            auto& bvh = tmt::engine.renderer.scene_view.bvh;

            if (bvh.nodes[0].left_first == 0u && bvh.nodes[0].prim_count == 0u) continue;

            for (auto& node : *nav_mesh.generating_nodes) {
                node.world_pos = glm::vec3(world_matrix * glm::vec4(node.local_pos, 1.0f));
            }
            nav_mesh.generate_mesh_over_time();
        }

        if (!nav_mesh.nodes_mesh) continue;

        for (auto& node : *nav_mesh.nodes_mesh) {
            node.world_pos = glm::vec3(world_matrix * glm::vec4(node.local_pos, 1.0f));
        }

        nav_mesh.inspect();
    }
}

void tmt::NavigationSystem::on_end() {}
