#include "chase_player.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/walking.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/engine.hpp"

#include "engine/core/polyline.hpp"

#include <cstdlib>

void ChasePlayer::on_start(tmt::Entity) {
    //
    for (const auto& [CamEntity, camera] : tmt::engine.ecs.get_registry().view<tmt::Camera>().each()) {
        player = CamEntity;
        break;
    }

    done_walking = false;
    has_path = false;
}

void ChasePlayer::on_tick(tmt::Entity walking_entity, float dt) {
    const auto& walking = tmt::engine.ecs.get_component<Walking>(walking_entity);
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(walking.walkable_asteroid);

    tmt::Transform& walking_transform = tmt::engine.ecs.get_component<tmt::Transform>(walking_entity);
    const auto& walking_entity_pos = walking_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    float dist = glm::length(player_pos - walking_entity_pos);

    // Stop chasing only if player is too far
    if (dist > walking.max_chase_distance) {
        auto& ecs = tmt::engine.ecs;
        auto& ws = ecs.get_component<tmt::WorldState>(walking_entity);
        ws.facts[std::hash<std::string>()("player_in_range")] = false;
        done_walking = true;
        return;
    }

    // Follow path to player
    std::optional<glm::vec3> direction = nav_mesh.follow_path(walking_entity_pos, player_pos);

    // If path has more waypoints, move to next one
    if (direction) {
        walking_transform.set_world_position(walking_entity_pos + *direction * walking.walk_speed * dt);
    } else {
        // Close enough to consider node reached, force path recompute
        nav_mesh.path.clear();
    }

    // Debug drawing
    if (nav_mesh.path.size() > 1) {
        for (size_t i = 0; i < nav_mesh.path.size() - 1; ++i) {
            glm::vec3 from = nav_mesh.nodes[nav_mesh.path[i]].world_pos;
            glm::vec3 to = nav_mesh.nodes[nav_mesh.path[i + 1]].world_pos;
            tmt::engine.polyline.draw_line(from, to);
        }
    }
}

bool ChasePlayer::is_done(tmt::Entity) const { return false; }
