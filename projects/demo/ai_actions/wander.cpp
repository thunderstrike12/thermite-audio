#include "wander.hpp"
#include "wander.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "../components/walking.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/engine.hpp"

#include "engine/core/polyline.hpp"

#include <cstdlib>

void Wander::on_start(tmt::Entity) {
    for (const auto& [camEntity, camera] : tmt::engine.ecs.get_registry().view<tmt::Camera>().each()) {
        player = camEntity;
        break;
    }

    done_walking = false;
    has_path = false;
}

void Wander::on_tick(tmt::Entity walking_entity, float dt) {
    const auto& walking = tmt::engine.ecs.get_component<Walking>(walking_entity);
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(walking.walkable_asteroid);

    tmt::Transform& walking_transform = tmt::engine.ecs.get_component<tmt::Transform>(walking_entity);
    const auto& walking_entity_pos = walking_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    float dist = glm::length(player_pos - walking_entity_pos);

    // in chase range, update world state
    if (dist < walking.max_chase_distance) {
        auto& ecs = tmt::engine.ecs;
        auto& ws = ecs.get_component<tmt::WorldState>(walking_entity);
        ws.facts[std::hash<std::string>()("player_in_range")] = true;
        done_walking = true;
        return;
    }

    auto nm_nodes = nav_mesh.nodes;

    // generate wander path only once when no path
    if (!has_path) {
        // pick a random node on the nav mesh
        if (!nav_mesh.nodes->empty()) {
            int random_node_index = rand() % static_cast<int>(nav_mesh.nodes->size());
            wander_target = (*nm_nodes)[random_node_index].world_pos;
            has_path = true;
        }
    }

    // follow path toward wander target
    if (has_path) {
        std::optional<glm::vec3> direction = nav_mesh.follow_path(walking_entity_pos, wander_target);

        if (!direction) {
            has_path = false;  // need a new random destination
            return;
        }

        walking_transform.set_world_position(walking_entity_pos + *direction * walking.walk_speed * dt);
    }

    // Debug drawing
    if (nav_mesh.path.size() > 1) {
        for (size_t i = 0; i < nav_mesh.path.size() - 1; ++i) {
            const glm::vec3 from = (*nm_nodes)[nav_mesh.path[i]].world_pos;
            const glm::vec3 to = (*nm_nodes)[nav_mesh.path[i + 1]].world_pos;
            tmt::engine.polyline.draw_line(from, to);
        }
    }
}

bool Wander::is_done(tmt::Entity) const {
    return false;
}
