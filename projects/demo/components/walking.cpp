#include "walking.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"

void Walking::start() {
    // tmt::GoapAgentFactory::spawn_agent_from_type("dragon", entity);
}

void Walking::update(const tmt::FrameData& time) {
    tmt::engine.polyline.use_color(1.0f, 0.0f, 0.0f);
    tmt::engine.polyline.use_line_width(2.0f);
    tmt::engine.polyline.use_depth_testing(false);
    tmt::Transform& walking_transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

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

    glm::vec3 move_to = ray.origin + ray.dir * hit.distance - ray.dir * height_above_ground;
    glm::vec3 desired_velocity = glm::normalize(move_to - walking_transform.get_world_position()) * walk_speed * 0.5f;
    if (glm::isnan(desired_velocity.x)) {
        tmt::Log::warn("desired vel is nan");
        return;
    }

    velocity += desired_velocity;

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

    // compute end effector positions
    // Extract object basis from its rotation
    glm::vec3 forward = glm::normalize(rotation * glm::vec3(0, 0, 1));
    glm::vec3 right = glm::normalize(rotation * glm::vec3(1, 0, 0));
    forward -= ground_up * glm::dot(forward, ground_up);
    right -= ground_up * glm::dot(right, ground_up);
    forward = glm::normalize(forward);
    right = glm::normalize(right);
}

void Walking::end() {}
