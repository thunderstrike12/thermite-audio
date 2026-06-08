#include "steering_system.hpp"

#include <glm/gtx/norm.hpp>
#include <glm/gtc/random.hpp>

#include "engine.hpp"
#include "core/logger.hpp"
#include "engine/core/polyline.hpp"

#include "systems/physics/physics_system.hpp"

namespace tmt {

void SteeringSystem::on_start() {}

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

        // Gravity gun or other external system overrides steering completely
        if (agent.force_override) {
            glm::vec3 target = agent.override_force;

            // smooth toward target instead of snapping
            body.velocity = glm::mix(glm::vec3(body.velocity), glm::vec3(target), 0.2f);
            continue;
        }

        body.type = VoxelBody::DYNAMIC;

        check_completion(agent, request, transform, body);

        if (request.mode == SteeringMode::NONE) continue;

        // Ge origin of the agent entity for wander behavior
        if (!agent.has_origin) {
            agent.wander_origin = transform.get_world_position();
            agent.has_origin = true;
        }

        glm::vec3 steering = calculate_force(agent, request, transform, body, time.delta_time);

        // separating from other steering agents
        glm::vec3 position = transform.get_world_position();
        steering += separation(entity, agent, position);

        // Clamp steering acceleration
        float len = glm::length(steering);
        if (len > agent.max_force) steering = (steering / len) * agent.max_force;

        // Apply steering as acceleration
        body.velocity += steering * time.delta_time;

        // Clamp max speed
        float speed = glm::length(body.velocity);
        if (speed > agent.max_speed) body.velocity = (body.velocity / speed) * agent.max_speed;

        // rotate towards movement
        glm::vec3 desired_forward = body.velocity;
        desired_forward.y = 0.0f;

        // Ignore tiny unstable velocities caused by collisions/explosions
        if (glm::length2(desired_forward) < 0.05f) {
            continue;
        }

        desired_forward = glm::normalize(desired_forward);

        glm::quat target_rot = glm::quatLookAt(desired_forward, glm::vec3(0, 1, 0));

        target_rot *= glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0));

        glm::quat current_rot = body.rotation;

        float turn_speed = 5.0f;
        glm::quat new_rot = glm::slerp(current_rot, target_rot, turn_speed * time.delta_time);

        body.rotation = new_rot;
    }
}

/**
 * Returns a steering force to move toward a target at max_speed.
 */
glm::vec3 SteeringSystem::seek(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target, const glm::vec3& velocity) {
    glm::vec3 desired = glm::normalize(target - pos) * agent.max_speed;

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

    float speed = agent.max_speed;

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
        velocity_dir = glm::vec3(0, 0, 1);
    }

    // --- Normal wander ---
    glm::vec3 jitter(
        glm::linearRand(-1.0f, 1.0f) * wander.wander_jitter * dt, glm::linearRand(-1.0f, 1.0f) * wander.wander_jitter * dt, glm::linearRand(-1.0f, 1.0f) * wander.wander_jitter * dt
    );

    wander.wander_target += jitter;
    wander.wander_target = glm::normalize(wander.wander_target) * wander.wander_radius;

    glm::vec3 target_in_front = velocity_dir * wander.wander_distance;
    glm::vec3 world_target = position + target_in_front + wander.wander_target;

    glm::vec3 desired_velocity = glm::normalize(world_target - position) * agent.max_speed;

    glm::vec3 wander_force = desired_velocity - body.velocity;

    // --- Boundary constraint ---
    glm::vec3 to_origin = agent.wander_origin - position;
    float dist = glm::length(to_origin);

    float max_radius = agent.wander_radius_limit;

    if (dist > max_radius) {
        // Strong pull back toward center
        glm::vec3 return_desired = glm::normalize(to_origin) * agent.max_speed;

        glm::vec3 return_force = return_desired - body.velocity;

        // Blend: stronger the further you are outside
        float strength = (dist - max_radius) / max_radius;
        strength = glm::clamp(strength, 0.0f, 1.0f);

        wander_force = glm::mix(wander_force, return_force, strength);
    }

    return wander_force;
}

/**
 * Returns a steering force to avoid obstacles in 3D space.
 *
 * Steps:
 *  - Treat the agent as a small volume by offsetting ray origins around its body
 *    (center, left/right sides, top/bottom).
 *  - Cast multiple rays from each origin: forward, forward+right, forward-right, forward+up, forward-up.
 *  - Cast rays to check for collisions (excluding the enemy layer).
 *  - For each hit within avoidDistance, accumulate a repelling force proportional to proximity.
 *  - If all rays from all origins are blocked, do a los check to either keep going or block
 */
glm::vec3 SteeringSystem::collision_avoidance(const SteeringAgent& agent, const glm::vec3& position, const VoxelBody& body, glm::vec3 dir) {
    auto* physics = engine.ecs.systems.try_get<Physics>();
    if (!physics) return glm::vec3(0);

    all_rays_blocked = false;

    if (glm::length2(body.velocity) < 0.0001f) return glm::vec3(0);

    glm::vec3 desired = dir;

    if (glm::length2(desired) < 0.0001f) return glm::vec3(0);

    glm::vec3 forward = glm::normalize(desired);
    glm::vec3 world_up(0, 1, 0);

    glm::vec3 right = glm::normalize(glm::cross(forward, world_up));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    uint32_t layer_mask = 0xFFFFFFFF & ~(1 << 2);  // ignore enemies

    float avoid_distance = 15.0f;

    float back_offset = 1.5f;
    glm::vec3 base = position - forward * back_offset;

    std::vector<std::pair<glm::vec3, glm::vec3>> rays = {
        { base, forward }, { base - right * 0.8f, forward }, { base + right * 0.8f, forward }, { base + up * 1.2f, forward }, { base - up * 1.2f, forward }
    };

    glm::vec3 avoid(0.0f);
    int blocked = 0;

    for (auto& r : rays) {
        Ray ray(r.first, r.second);
        Hit hit = physics->raycast(ray, layer_mask);

        /*engine.polyline.use_color(1.0f, 0.0f, 0.0f);
        engine.polyline.use_line_width(0.5f);
        engine.polyline.draw_line(ray.origin, ray.origin + ray.dir * avoid_distance);*/

        if (hit) {
            float t = 1.0f - (hit.distance / avoid_distance);
            t = glm::clamp(t, 0.0f, 1.0f);

            float strength = agent.max_force * t;
            glm::vec3 side = glm::normalize(glm::cross(world_up, forward));

            avoid += side * strength;

            blocked++;
        }
    }

    if (blocked == (int)rays.size()) {
        all_rays_blocked = true;
    }

    if (glm::length2(avoid) < 0.01f) avoid = glm::vec3(0.0f);

    return avoid;
}

/**
 * Returns a force to avoid other small enemies.
 *
 * Steps:
 *  - Find other steering agent, and get their position.
 *  - Mov away from the enemy with a force which is stronger the closer you are.
 */
glm::vec3 SteeringSystem::separation(entt::entity self, const SteeringAgent& agent, const glm::vec3& position) {
    glm::vec3 force(0.0f);

    auto view = engine.ecs.view<SteeringAgent, Transform>();

    const float desired_distance = 3.0f;
    const float desired_distance2 = desired_distance * desired_distance;

    for (auto [other, other_agent, other_transform] : view.each()) {
        // exclude self using entity id
        if (other == self) continue;

        glm::vec3 diff = position - other_transform.get_world_position();

        float dist2 = glm::length2(diff);

        if (dist2 < 0.0001f) continue;

        // only affect nearby agents
        if (dist2 < desired_distance2) {
            float dist = sqrt(dist2);

            // stronger push when closer
            glm::vec3 dir = diff / dist;

            force += dir / dist;
        }
    }

    return force * agent.max_force;
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

    glm::vec3 to_target = request.target_position - position;
    float dist_to_target = glm::length(to_target);

    glm::vec3 dir_to_target = (dist_to_target > 0.0001f) ? to_target / dist_to_target : glm::vec3(0.0f);

    auto* physics = engine.ecs.systems.try_get<Physics>();

    bool has_line_of_sight = true;

    uint32_t layer_mask = 0xFFFFFFFF & ~(1 << 2);  // ignore enemies

    if (physics) {
        Ray ray(position, dir_to_target);
        Hit hit = physics->raycast(ray, layer_mask);

        if (hit && hit.distance < dist_to_target) {
            has_line_of_sight = false;
        }
    }

    glm::vec3 force(0);

    switch (request.mode) {
        case SteeringMode::SEEK:
            force += seek(agent, position, request.target_position, body.velocity);
            break;

        case SteeringMode::ARRIVE:
            force += arrive(agent, position, request.target_position, agent.arrive_radius, body.velocity);
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
    glm::vec3 avoid = collision_avoidance(agent, position, body, dir_to_target);

    // If we can see the player, and not all the rays are blocked, move towards player
    if (has_line_of_sight && all_rays_blocked) {
        avoid *= 0.f;   // no avoidance
    } else {
        avoid *= 1.0f;  // full avoidance when path is blocked
    }

    force += avoid;

    return force;
}

/**
 * For ARRIVE requests, checks if the agent is within arrive_radius and moving slowly enough.
 * Stops the agent and marks the request as completed.
 */
void SteeringSystem::check_completion(const SteeringAgent& agent, SteeringRequest& request, const Transform& transform, VoxelBody& body) {
    if (request.mode != SteeringMode::ARRIVE) return;

    float dist = glm::distance(transform.get_world_position(), request.target_position);

    // stop movement if close enough
    if (dist <= agent.min_explosion_range) {
        body.velocity = glm::vec3(0);
        body.angular_velocity = glm::vec3(0);

        request.completed = true;           // mark as completed
        request.mode = SteeringMode::NONE;  // stop steering, but keep the component
    }
}

}  // namespace tmt
