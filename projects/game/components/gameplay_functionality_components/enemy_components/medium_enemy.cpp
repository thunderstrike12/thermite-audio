#include "medium_enemy.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"

#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"

void game::MediumEnemy::start() {
    for (const auto& [CamEntity, camera] : tmt::engine.ecs.view<tmt::Camera>().each()) {
        player = CamEntity;
        break;
    }

    auto& dispatcher = tmt::engine.ecs.get_dispatcher();
    dispatcher.sink<game::GamePausedEvent>().connect<&MediumEnemy::on_game_paused>(this);
    dispatcher.sink<game::GameUnpausedEvent>().connect<&MediumEnemy::on_game_unpaused>(this);
}

void game::MediumEnemy::update(const tmt::FrameData& time) {
    if (paused) return;

    // check core, if none, enemy dies
    // if (this->core == entt::null) die();

    tmt::engine.polyline.use_color(1.0f, 0.0f, 0.0f);
    tmt::engine.polyline.use_line_width(2.0f);
    tmt::engine.polyline.use_depth_testing(false);
    tmt::Transform& walking_transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    // world state update
    auto ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(entity);
    if (!ws) return;
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();
    float dist = glm::length(player_pos - walking_transform.get_world_position());

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
    auto nodes = nav_mesh.nodes_mesh;
    if (nodes->empty()) return;
    int closest_node = nav_mesh.find_closest_node(walking_transform.get_world_position());

    auto& normal = (*nodes)[closest_node].normal;

    tmt::Ray ray;
    ray.origin = walking_transform.get_world_position() + normal * 0.5f;
    ray.dir = glm::normalize(-normal);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray);

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
        float t = time.delta_time * rotation_speed;
        rotation = glm::slerp(rotation, rot_velocity, glm::clamp(t, 0.0f, 1.0f));
        rotation = glm::normalize(rotation);
        walking_transform.set_world_rotation(rotation);
    }

    // missile update
    for (int i = missiles.size() - 1; i >= 0; i--) {
        if (missiles[i].update(time.delta_time, ground_up, player_pos)) {
            missiles.erase(missiles.begin() + i);
        }
    }
}

void game::MediumEnemy::end() {}

void game::MediumEnemy::kite_player() const {
    auto& enemy = tmt::engine.ecs.get_component<MediumEnemy>(entity);
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(enemy.walkable_asteroid);

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();
    float distance = glm::distance(enemy_entity_pos, player_pos);

    if (std::optional<glm::vec3> direction = nav_mesh.follow_path(enemy_entity_pos, player_pos)) {
        if (distance < back_off_distance) {
            glm::vec3 target_pos = enemy_entity_pos - *direction;
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

void game::MediumEnemy::on_game_paused(const game::GamePausedEvent&) {
    paused = true;
}

void game::MediumEnemy::on_game_unpaused(const game::GameUnpausedEvent&) {
    paused = false;
}

bool Missile::update(float dt, glm::vec3 ground_up, glm::vec3 player_pos) {
    tmt::engine.polyline.draw_sphere(position, 0.1f);
    life_time += dt;
    if (life_time > max_life_time) return true;

    // fades from full launch down to zero
    float launch_t = glm::clamp((life_time / stop_launching_after), 0.0f, 1.0f);
    glm::vec3 launch_contribution = glm::mix(ground_up * launch_speed, glm::vec3(0.0f), launch_t);

    // fades from zero up to full homing speed
    glm::vec3 home_contribution(0.0f);
    if (life_time > start_homing_after) {
        float max_home_time = max_life_time - start_homing_after;
        float home_time = life_time - start_homing_after;
        float home_t = glm::clamp(home_time / max_home_time, 0.0f, 1.0f);  // was inverted

        glm::vec3 to_player = player_pos - position;
        if (glm::dot(to_player, to_player) > 0.0001f) {
            home_contribution = glm::normalize(to_player) * home_speed * home_t;
        }
    }

    auto added_vel = launch_contribution + home_contribution;
    float length = glm::length(added_vel);
    if (length > 0.0001f && life_time < start_homing_after) {
        added_vel += offset;
        added_vel = glm::normalize(added_vel) * length;
    }
    velocity += added_vel;
    velocity *= 0.7f;
    position += velocity * dt;
    return false;
}
