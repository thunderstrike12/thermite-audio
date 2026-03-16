#include "wander.hpp"
#include "wander.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/engine.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"


#include <cstdlib>

void Wander::on_start(tmt::Entity enemy_entity) {
    has_path = false;
}

void Wander::on_tick(tmt::Entity enemy_entity, float dt) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(enemy.walkable_asteroid);

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();

    auto nm_nodes = nav_mesh.nodes_mesh;

    // generate wander path only once when no path
    if (!has_path && nav_mesh.nodes_mesh) {
        // pick a random node on the nav mesh
        if (!nav_mesh.nodes_mesh->empty()) {
            int random_node_index = rand() % static_cast<int>(nav_mesh.nodes_mesh->size());
            wander_target = (*nm_nodes)[random_node_index].world_pos;
            has_path = true;
        }
    }

    // follow path toward wander target
    if (has_path) {
        std::optional<glm::vec3> direction = nav_mesh.follow_path(enemy_entity_pos, wander_target);

        auto velocity = glm::vec3(*direction * enemy.walk_speed);
        enemy.velocity += velocity;
        if (glm::abs(enemy.velocity.x) < 0.1f && glm::abs(enemy.velocity.y) < 0.1f && glm::abs(enemy.velocity.z) < 0.1f) {
            has_path = false;  // need a new random destination
            return;
        }
    }
}

bool Wander::is_done(tmt::Entity) const {
    return false;
}
