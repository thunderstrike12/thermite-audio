#include "constraint_solver.hpp"
#include "systems/physics/components/voxel_body.hpp"
#include "engine.hpp"
#include <glm/gtx/norm.hpp>

using namespace tmt;

void ConstraintSolver::solve_velocities(const float) {
    const size_t collision_count = contact_index;
    // Solve velocity constraints
    for (size_t i = 0; i < velocity_iterations; i++) {
        for (size_t j = 0; j < collision_count; j++) {
            auto& collision = collisions[j];
            // Get the rigid bodies involved in the constraint
            auto& body_a = engine.ecs.get_registry().get<VoxelBody>(collision.entity_a);
            auto& body_b = engine.ecs.get_registry().get<VoxelBody>(collision.entity_b);

            const float inv_mass_a = body_a.get_inv_mass();
            const float inv_mass_b = body_b.get_inv_mass();

            // Transform inertia tensors to world space
            const glm::mat3 inv_inertia_a_world = body_a.get_inv_world_inertia();
            const glm::mat3 inv_inertia_b_world = body_b.get_inv_world_inertia();

            // const float restitution = body_a.elasticity * body_b.elasticity;
            // const float friction = std::min(body_a.friction, body_b.friction);

            for (auto& contact : collision.contacts) {
                const glm::vec3 r_a = contact.point - body_a.center_of_mass;
                const glm::vec3 r_b = contact.point - body_b.center_of_mass;

                const glm::vec3 r_a_cross_n = glm::cross(r_a, contact.normal);
                const glm::vec3 r_b_cross_n = glm::cross(r_b, contact.normal);

                const glm::vec3 i_cross_a = inv_inertia_a_world * r_a_cross_n;
                const glm::vec3 i_cross_b = inv_inertia_b_world * r_b_cross_n;

                float effective_mass = 1.0f / (inv_mass_a + inv_mass_b + glm::dot(i_cross_a, r_a_cross_n) + glm::dot(i_cross_b, r_b_cross_n));
                contact.normal_mass = effective_mass;

                const float jv = glm::dot(contact.normal, body_a.velocity - body_b.velocity) + glm::dot(r_a_cross_n, body_a.angular_velocity) - glm::dot(r_b_cross_n, body_b.angular_velocity);

                float impulse_delta = (1.0f + RESTITUTION) * jv * effective_mass;

                const float new_impulse = std::max(contact.normal_impulse + impulse_delta, 0.0f);
                impulse_delta = new_impulse - contact.normal_impulse;
                contact.normal_impulse = new_impulse;

                const glm::vec3 impulse = impulse_delta * contact.normal;

                // Linear velocity updates
                body_a.velocity -= impulse * inv_mass_a;
                body_b.velocity += impulse * inv_mass_b;

                // Angular velocity updates
                body_a.angular_velocity -= impulse_delta * i_cross_a;
                body_b.angular_velocity += impulse_delta * i_cross_b;
            }

            for (auto& contact : collision.contacts) {
                glm::vec3 r_a = contact.point - body_a.center_of_mass;
                glm::vec3 r_b = contact.point - body_b.center_of_mass;

                // Friction calculation
                glm::vec3 relative_velocity = body_b.velocity + glm::cross(body_b.angular_velocity, r_b) - body_a.velocity - glm::cross(body_a.angular_velocity, r_a);
                glm::vec3 tangent = relative_velocity - glm::dot(relative_velocity, contact.normal) * contact.normal;

                if (glm::length2(tangent) > 0.0001f) {
                    tangent = glm::normalize(tangent);

                    glm::vec3 r_a_cross_t = glm::cross(r_a, tangent);
                    glm::vec3 r_b_cross_t = glm::cross(r_b, tangent);

                    float friction_mass =
                        1.0f / (inv_mass_a + inv_mass_b + glm::dot(inv_inertia_a_world * r_a_cross_t, r_a_cross_t) + glm::dot(inv_inertia_b_world * r_b_cross_t, r_b_cross_t));

                    float friction_impulse = -glm::dot(relative_velocity, tangent) * friction_mass;

                    float max_friction = contact.normal_impulse * FRICTION;
                    float new_friction_impulse = glm::clamp(contact.tangent_impulse + friction_impulse, -max_friction, max_friction);
                    friction_impulse = new_friction_impulse - contact.tangent_impulse;
                    contact.tangent_impulse = new_friction_impulse;

                    glm::vec3 friction_force = friction_impulse * tangent;

                    // Apply friction impulse
                    body_a.velocity -= friction_force * inv_mass_a;
                    body_b.velocity += friction_force * inv_mass_b;

                    body_a.angular_velocity -= inv_inertia_a_world * glm::cross(r_a, friction_force);
                    body_b.angular_velocity += inv_inertia_b_world * glm::cross(r_b, friction_force);
                }
            }
        }
    }
}

void ConstraintSolver::solve_positions(const float) {
    const size_t collision_count = contact_index;
    for (size_t i = 0; i < position_iterations; i++) {
        for (size_t j = 0; j < collision_count; j++) {
            auto& collision = collisions[j];
            // Get the rigid bodies involved in the constraint
            auto& body_a = engine.ecs.get_registry().get<VoxelBody>(collision.entity_a);
            auto& body_b = engine.ecs.get_registry().get<VoxelBody>(collision.entity_b);

            for (auto& contact : collision.contacts) {
                const float steering_constant = 0.005f;
                const float max_correction = -VOXEL_SIZE_HALF;
                const float slop = VOXEL_SIZE_SQR;

                const float steering_force = glm::clamp(steering_constant * (-contact.penetration + slop), max_correction, 0.0f);
                const glm::vec3 impulse = contact.normal * (-steering_force * contact.normal_mass);

                if (body_a.type != VoxelBody::STATIC) {
                    body_a.center_of_mass -= impulse * body_a.inv_mass;
                    glm::vec3 r_a = contact.point - body_a.center_of_mass;
                    glm::vec3 axis = glm::cross(r_a, -impulse);

                    glm::quat rot = glm::angleAxis(glm::length(impulse * body_a.get_inv_world_inertia()), axis);
                    body_a.rotation = glm::normalize(rot * body_a.rotation);
                }

                if (body_b.type != VoxelBody::STATIC) {
                    body_b.center_of_mass += impulse * body_b.inv_mass;
                    glm::vec3 r_b = contact.point - body_b.center_of_mass;
                    glm::vec3 axis = glm::cross(r_b, impulse);

                    glm::quat rot = glm::angleAxis(glm::length(impulse * body_b.get_inv_world_inertia()), axis);
                    body_b.rotation = glm::normalize(rot * body_b.rotation);
                }
            }

            // Update the positions of the rigid bodies
            if (body_a.type != VoxelBody::STATIC) {
                body_a.position = body_a.center_of_mass - body_a.rotation * body_a.com_local_offset;
            }

            if (body_b.type != VoxelBody::STATIC) {
                body_b.position = body_b.center_of_mass - body_b.rotation * body_b.com_local_offset;
            }
        }
    }

    collisions.clear();
    contact_index = 0;
}
