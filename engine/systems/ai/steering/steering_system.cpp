#include "steering_system.hpp"

#include <glm/gtx/norm.hpp>
#include <glm/gtc/random.hpp>

#include "engine.hpp"
#include "core/logger.hpp"
#include "engine/core/polyline.hpp"

#include "systems/physics/physics_system.hpp"

namespace tmt {

void SteeringSystem::on_start() {
    Log::info("Steering on_start");

    overrides().load();

    auto view = engine.ecs.view<SteeringAgent>();
    for (auto [entity, agent] : view.each()) {
        agent.params = &overrides().params;
    }
}

/**
 * Main per-frame physics update for steering agents.
 *
 * Steps:
 *  - Iterate all entities with SteeringAgent, SteeringRequest, Transform, VoxelBody.
 *  - Calculate total steering force using calculate_force().
 *  - Apply steering as acceleration to the velocity.
 */
void SteeringSystem::on_fixed_update(const FrameData& time) {
    auto view = engine.ecs.view<SteeringAgent, SteeringRequest, Transform, VoxelBody>();

    for (auto [entity, agent, request, transform, body] : view.each()) {
        if (!agent.active) continue;

        body.type = VoxelBody::DYNAMIC;
        if (body.type != VoxelBody::DYNAMIC) continue;

        if (request.mode == SteeringMode::NONE) continue;

        //// -------- Overshoot prevention --------
        // glm::vec3 position = transform.get_world_position();

        // if (request.mode == SteeringMode::ARRIVE) {
        //     glm::vec3 to_target = request.target_position - position;

        //    float dist = glm::length(to_target);
        //    float move_this_frame = glm::length(body.velocity) * time.delta_time;

        //    if (move_this_frame >= dist) {
        //        body.velocity = glm::vec3(0);
        //        body.angular_velocity = glm::vec3(0);

        //        request.completed = true;
        //        request.mode = SteeringMode::NONE;

        //        continue;
        //    }
        //}
        //// --------------------------------------

        glm::vec3 steering = calculate_force(agent, request, transform, body, time.delta_time);

        // Clamp steering acceleration
        float len = glm::length(steering);
        if (len > agent.params->max_force) steering = (steering / len) * agent.params->max_force;

        // Apply steering as acceleration
        body.velocity += steering * time.delta_time;

        // Clamp max speed
        float speed = glm::length(body.velocity);
        if (speed > agent.params->max_speed) body.velocity = (body.velocity / speed) * agent.params->max_speed;

        check_completion(request, transform, body);
    }
}

/**
 * Returns a steering force to move toward a target at max_speed.
 */
glm::vec3 SteeringSystem::seek(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target, const glm::vec3& velocity) {
    glm::vec3 desired = glm::normalize(target - pos) * agent.params->max_speed;

    return desired - velocity;
}

/**
 * Returns a steering force to move toward a target but slow down near it.
 *
 * Note:
 *  - radius = Distance from target where the agent begins slowing.
 *
 * Returns:
 *  - Vector representing desired change in velocity.
 *  - Or -velocity to brake if very close.
 */
glm::vec3 SteeringSystem::arrive(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target, float radius, const glm::vec3& velocity) {
    glm::vec3 to_target = target - pos;
    float dist = glm::length(to_target);

    if (dist < 0.01f) return -velocity;  // brake

    float speed = agent.params->max_speed;

    if (dist < radius) speed *= (dist / radius);

    glm::vec3 desired = glm::normalize(to_target) * speed;

    return desired - velocity;
}

/**
 * Returns a wandering steering force, introducing small random changes to direction.
 *
 * Steps:
 *  - Add random jitter to wanderTarget.
 *  - project wanderTarget to a sphere of wanderRadius.
 *  - Project target forward by wanderDistance.
 *  - Calculate desired velocity toward the resulting world target.
 */
glm::vec3 SteeringSystem::wander(const SteeringAgent& agent, const glm::vec3& position, const VoxelBody& body, WanderData wander, float dt) {
    glm::vec3 velocity_dir = glm::normalize(body.velocity);
    if (glm::length2(body.velocity) < 0.0001f) {
        // if agent is almost stationary, pick some default forward
        velocity_dir = glm::vec3(0, 0, 1);
    }

    // Add random jitter to wanderTarget
    glm::vec3 jitter(
        (glm::linearRand(-1.0f, 1.0f)) * wander.wander_jitter * dt, (glm::linearRand(-1.0f, 1.0f)) * wander.wander_jitter * dt, (glm::linearRand(-1.0f, 1.0f)) * wander.wander_jitter * dt
    );
    wander.wander_target += jitter;

    wander.wander_target = glm::normalize(wander.wander_target) * wander.wander_radius;

    // Calculate target in world space
    glm::vec3 target_in_front = velocity_dir * wander.wander_distance;
    glm::vec3 world_target = position + target_in_front + wander.wander_target;

    // Seek toward worldTarget
    glm::vec3 desired_velocity = glm::normalize(world_target - position) * agent.params->max_speed;
    return desired_velocity - body.velocity;
}

/**
 * Returns a steering force to avoid obstacles in 3D space.
 *
 * Steps:
 *  - Cast multiple rays: forward, forward+right, forward-right, forward+up, forward-up.
 *  - Query the Physics system to detect hits, ignoring the enemy layer.
 *  - For each hit within avoidDistance, accumulate a repelling force proportional to proximity.
 */
glm::vec3 SteeringSystem::collision_avoidance(const SteeringAgent& agent, const glm::vec3& position, const VoxelBody& body) {
    if (glm::length2(body.velocity) < 0.0001f) return glm::vec3(0);

    glm::vec3 forward = glm::normalize(body.velocity);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    auto* physics = engine.ecs.systems.try_get<Physics>();
    if (!physics) return glm::vec3(0);

    uint32_t layer_mask = 0xFFFFFFFF & ~(1 << 2) & ~(1 << 1);  // ignore enemies and player
    float avoid_distance = 30.0f;

    glm::vec3 total_avoid(0.0f);

    float side_factor = 0.2f;  // smaller deviation for side/up rays

    std::vector<glm::vec3> rays = { forward, glm::normalize(forward + right * side_factor), glm::normalize(forward - right * side_factor), glm::normalize(forward + up * side_factor),
                                    glm::normalize(forward - up * side_factor) };

    for (auto dir : rays) {
        Ray ray(position, dir);
        Hit hit = physics->raycast(ray, layer_mask);

        // debug draw
        /*engine.polyline.use_color(1.0f, 0.0f, 0.0f);
        engine.polyline.use_line_width(0.5f);
        engine.polyline.draw_line(ray.origin, ray.origin + dir * avoid_distance);*/

        if (hit && hit.distance < avoid_distance) {
            float strength = agent.params->max_force * (avoid_distance - hit.distance) / avoid_distance;
            // total_avoid += hit.normal * strength;
            total_avoid += -dir * strength;
        }
    }

    return total_avoid;
}

/**
 * Combines the requested steering behavior with obstacle avoidance.
 *
 * Steps:
 *  - Determine base force from the current request mode (SEEK, ARRIVE, FLEE, WANDER).
 *  - Add collision_avoidance force on top.
 */
glm::vec3 SteeringSystem::calculate_force(const SteeringAgent& agent, const SteeringRequest& request, const Transform& transform, const VoxelBody& body, float dt) {
    glm::vec3 position = transform.get_world_position();

    glm::vec3 force(0);

    switch (request.mode) {
        case SteeringMode::SEEK:
            force += seek(agent, position, request.target_position, body.velocity);
            break;

        case SteeringMode::ARRIVE:
            force += arrive(agent, position, request.target_position, overrides().params.arrive_radius, body.velocity);
            break;

        case SteeringMode::FLEE:
            force += -seek(agent, position, request.target_position, body.velocity);
            break;

        case SteeringMode::WANDER:
            force += wander(agent, position, body, request.wander_data, dt);
            dt;
            break;

        default:
            break;
    }

    // Add avoidance on top
    force += collision_avoidance(agent, position, body);

    return force;
}

/**
 * For ARRIVE requests, checks if the agent is within arrive_radius and moving slowly enough.
 * Stops the agent and marks the request as completed.
 */
void SteeringSystem::check_completion(SteeringRequest& request, const Transform& transform, VoxelBody& body) {
    if (request.mode != SteeringMode::ARRIVE) return;

    float dist = glm::distance(transform.get_world_position(), request.target_position);

    // stop movement if close enough
    if (dist <= overrides().params.min_explosion_range) {
        body.velocity = glm::vec3(0);
        body.angular_velocity = glm::vec3(0);

        request.completed = true;           // mark as completed
        request.mode = SteeringMode::NONE;  // stop steering, but keep the component
    }
}

}  // namespace tmt
