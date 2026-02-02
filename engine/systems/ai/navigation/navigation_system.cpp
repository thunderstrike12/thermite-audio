#include "navigation_system.hpp"

#include "engine/engine.hpp"

std::string tmt::NavigationSystem::get_name() { return "NavigationSystem"; }

void tmt::NavigationSystem::on_start() {
    for (const auto& [entity, nav_mesh] : engine.ecs.get_registry().view<NavMesh>().each()) {
        nav_mesh.generate_mesh();
    }
    for (const auto& [entity, nav_mesh] : engine.ecs.get_registry().view<NavMesh>().each()) {
        glm::mat4 world_matrix = engine.ecs.get_component<Transform>(entity).get_world_matrix();
        for (auto& node : nav_mesh.nodes) {
            node.world_pos = glm::vec3(world_matrix * glm::vec4(node.local_pos, 1.0f));
        }

        nav_mesh.inspect();
    }
}

void tmt::NavigationSystem::on_update(const FrameData&) {
    for (const auto& [entity, nav_mesh] : engine.ecs.get_registry().view<NavMesh>().each()) {
        glm::mat4 world_matrix = engine.ecs.get_component<Transform>(entity).get_world_matrix();
        for (auto& node : nav_mesh.nodes) {
            node.world_pos = glm::vec3(world_matrix * glm::vec4(node.local_pos, 1.0f));
        }

        nav_mesh.inspect();
    }
}

void tmt::NavigationSystem::on_end() {}
