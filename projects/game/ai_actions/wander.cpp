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

    if (!nav_mesh.nav_nodes_valid()) {
        tmt::Log::warn("Nav mesh not generated yet for wander action!");
        return;
    }
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
    tmt::engine.polyline.use_depth_testing(false);
    tmt::engine.polyline.draw_sphere(wander_target, 0.1f, 8, 0.1f);
    if (has_path) {
        std::optional<glm::vec3> direction = nav_mesh.follow_path(enemy_entity_pos, wander_target);
        if (direction) {
            auto velocity = glm::vec3(*direction * enemy.walk_speed);
            if (glm::length(velocity) > 10.0f) tmt::Log::warn("Wander velocity is very high: {}", glm::length(velocity));
            enemy.velocity += velocity;
        }
        if (glm::abs(enemy.velocity.x) < 0.1f && glm::abs(enemy.velocity.y) < 0.1f && glm::abs(enemy.velocity.z) < 0.1f) {
            tmt::Log::warn("Wander path recomputed", glm::length(enemy.velocity));
            has_path = false;  // need a new random destination
            return;
        }
    }

    auto* audio_emitter = tmt::engine.ecs.try_get_component<tmt::AudioEmitter>(enemy_entity);
    if (audio_emitter != nullptr && !enemy.walk_instance.is_valid()) {
        enemy.walk_instance = audio_emitter->play(enemy.sounds.sound_walk);
        enemy.walk_instance.set_maximum_distance(enemy.aggro_range * 1.5f);  // Multiply be 1.5f to ensure the player can hear it even when at the edge of the range
    }
}

bool Wander::is_done(tmt::Entity) const {
    return false;
}
