#include "physics_system.hpp"
#include "core/logger.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/components/transform.hpp"
#include "components/voxel_body.hpp"

#include "glm/gtx/norm.hpp"
#include <glm/gtx/quaternion.hpp>

namespace tmt {

void Physics::on_start() { Log::info("Physics on_start"); }

void Physics::on_update(const FrameData&) {}

void Physics::on_fixed_update(const FrameData&) {
    // Update Forces
    for (const auto& [entity, vb, transform] : engine.ecs.get_registry().view<VoxelBody, Transform>().each()) {
        // if (!transform.is_enabled()) continue;

        // Add stored forces
        vb.velocity += vb.stored_velocity;
        vb.stored_velocity = glm::vec3(0);

        vb.angular_velocity += vb.stored_torque;
        vb.stored_torque = glm::vec3(0);

        // Add gravity
        vb.velocity += glm::vec3(0, -1, 0) * vb.gravity * Engine::Config::FIXED_TIME_STEP;

        // Add drag
        vb.velocity *= std::max(0.0f, 1.0f - vb.linear_drag * Engine::Config::FIXED_TIME_STEP);
        vb.angular_velocity *= std::max(0.0f, 1.0f - vb.angular_drag * Engine::Config::FIXED_TIME_STEP);

        if (vb.type == VoxelBody::STATIC) continue;
        if (vb.type == VoxelBody::SLEEPING) {
            vb.velocity = glm::vec3(0);
            vb.angular_velocity = glm::vec3(0);
            continue;
        }

        // Accumulate
        float forces = (glm::length2(vb.velocity / Engine::Config::FIXED_TIME_STEP) + glm::length2(vb.angular_velocity / Engine::Config::FIXED_TIME_STEP));
        if (std::isnan(forces)) forces = 0;
        vb.accumulated_forces = vb.accumulated_forces * 0.925f + forces * 0.075f;

        // practicly not moving
        if (vb.accumulated_forces <= 150.0f && forces <= 150.0f)
            vb.type = VoxelBody::WANTS_SLEEP;
        else
            vb.type = VoxelBody::DYNAMIC;
    }

    // Detect Colllisions
    // Generate Contacts
    const int numb_objects = (int)engine.ecs.get_registry().view<VoxelBody, Transform>().size_hint();
    generate_voxel_constraints_range(0, numb_objects);

    // Solve Velocities
    solver.solve_velocities(Engine::Config::FIXED_TIME_STEP);

    // Apply Velocities
    // Update Positions
    apply_velocities();

    // Solve Positions
    solver.solve_positions(Engine::Config::FIXED_TIME_STEP);
}

void Physics::on_end() { Log::info("Physics on_end"); }

void Physics::generate_voxel_constraints_range(const int index, const int size) {
    // auto& physics_layers = Engine.layers();
    const int min = index * size;
    const int max = (index + 1) * size;
    auto vox_view = engine.ecs.get_registry().view<VoxelBody, Transform>();

    int curr = -1;
    for (const auto& [entity, vb, transform] : vox_view.each()) {
        // if (!transform.is_enabled()) continue;
        curr++;
        if (curr < min || curr >= max) continue;
        if (vb.type == VoxelBody::STATIC || vb.type == VoxelBody::SLEEPING) continue;
        for (const auto& [other_entity, other_vb, other_transform] : vox_view.each()) {
            // if (!other_transform.is_enabled()) continue;
            if (entity == other_entity) continue;

            // if (!(physics_layers.collides_with_layer(layer, other_layer))) continue;

            if (sat_early_out(vb, other_vb)) continue;

            Collision coll(entity, other_entity);
            coll.contacts.reserve(64);

            // Find overlapping area's
            // A's local collision overlap
            glm::ivec3 min_a = {};
            glm::ivec3 max_a = {};
            get_overlap(vb, other_vb, min_a, max_a);

            for (size_t z = min_a.z; z < max_a.z; z++) {
                for (size_t y = min_a.y; y < max_a.y; y++) {
                    for (size_t x = min_a.x; x < max_a.x; x++) {
                        if (coll.contacts.size() >= max_contacts) continue;
                        check_neighbors(vb, other_vb, x, y, z, coll);
                    }
                }
            }

            if (coll.contacts.size() <= 0) continue;

            coll.normal = glm::normalize(other_vb.position - vb.position);
            solver.collisions.push_back(coll);
            // thread_collisions[index].push_back(coll);
        }
    }
}

bool Physics::sat_early_out(const VoxelBody& vb_a, const VoxelBody& vb_b) {
    auto vert_a = vb_a.get_world_bounds();
    auto vert_b = vb_b.get_world_bounds();
    auto axes_a = vb_a.get_axes();
    auto axes_b = vb_b.get_axes();

    // Check against all axes
    for (size_t i = 0; i < 3; i++) {
        float overlap;

        if (is_separated(axes_a.axis[i], vert_a, vert_b, overlap)) return true;
        if (is_separated(axes_b.axis[i], vert_a, vert_b, overlap)) return true;

        for (size_t j = 0; j < 3; j++)  // edge-edge
        {
            auto cross_axis = glm::normalize(glm::cross(axes_a.axis[i], axes_b.axis[j]));
            if (glm::length(cross_axis) > 0) {
                if (is_separated(cross_axis, vert_a, vert_b, overlap)) return true;
            }
        }
    }

    return false;
}

void Physics::get_overlap(const VoxelBody& vb_a, const VoxelBody& vb_b, glm::ivec3& min, glm::ivec3& max) {
    const glm::vec3 local_half_a = glm::vec3(vb_a.width, vb_a.height, vb_a.depth) * 0.5f;
    // const auto local_bounds_a = vox_a.get_local_bounds();
    const auto box_a = vb_a.get_world_bounds();
    const auto edges_a = vb_a.get_world_edges();

    const auto pos_a = vb_a.position;
    const auto rot_a = vb_a.rotation;

    const glm::vec3 local_half_b = glm::vec3(vb_b.width, vb_b.height, vb_b.depth) * 0.5f;
    // const auto local_bounds_b = vox_b.get_local_bounds();
    const auto box_b = vb_b.get_world_bounds();
    const auto edges_b = vb_b.get_world_edges();

    const auto pos_b = vb_b.position;
    const auto rot_b = vb_b.rotation;

    glm::vec3 t_min = glm::vec3(1e32f);
    glm::vec3 t_max = glm::vec3(-1e32f);

    // Check B vertices in A
    for (size_t i = 0; i < 8; i++) {
        const glm::vec3 relative_vertex = (box_b.vertex[i] - pos_a) * rot_a;

        // AABB
        if (abs(relative_vertex.x) > local_half_a.x || abs(relative_vertex.y) > local_half_a.y || abs(relative_vertex.z) > local_half_a.z) continue;

        // Check all axes for min and max values
        for (int j = 0; j < 3; j++) {
            if (relative_vertex[j] < t_min[j]) t_min[j] = relative_vertex[j];
            if (relative_vertex[j] > t_max[j]) t_max[j] = relative_vertex[j];
        }
    }

    // Check A vertices in B
    for (size_t i = 0; i < 8; i++) {
        const glm::vec3 relative_vertex = (box_a.vertex[i] - pos_b) * rot_b;

        // AABB
        if (abs(relative_vertex.x) > local_half_b.x || abs(relative_vertex.y) > local_half_b.y || abs(relative_vertex.z) > local_half_b.z) continue;

        const glm::vec3 transformed_vertex = (box_a.vertex[i] - pos_a) * rot_a;

        // Check all axes for min and max values
        for (int j = 0; j < 3; j++) {
            if (transformed_vertex[j] < t_min[j]) t_min[j] = transformed_vertex[j];
            if (transformed_vertex[j] > t_max[j]) t_max[j] = transformed_vertex[j];
        }
    }

    // Check B edges
    for (size_t i = 0; i < 12; i++) {
        const glm::vec3 v1 = (edges_b[i].start - pos_a) * rot_a;
        const glm::vec3 v2 = (edges_b[i].end - pos_a) * rot_a;

        const glm::vec3 diff = v2 - v1;
        const float length = glm::length(diff);

        const float t1 = ray_aabb(-local_half_a, local_half_a, v1, glm::normalize(diff));
        // If ray hit
        if (t1 <= length && t1 > -0.001f) {
            glm::vec3 p = v1 + glm::normalize(v2 - v1) * t1;

            // Check all axes for min and max values
            for (int j = 0; j < 3; j++) {
                if (p[j] < t_min[j]) t_min[j] = p[j];
                if (p[j] > t_max[j]) t_max[j] = p[j];
            }
        }

        const float t2 = ray_aabb(-local_half_a, local_half_a, v2, glm::normalize(v1 - v2));
        // If ray hit
        if (t2 <= length && t2 > -0.001f) {
            glm::vec3 p = v2 + glm::normalize(v1 - v2) * t2;

            // Check all axes for min and max values
            for (int j = 0; j < 3; j++) {
                if (p[j] < t_min[j]) t_min[j] = p[j];
                if (p[j] > t_max[j]) t_max[j] = p[j];
            }
        }
    }

    // Check A edges
    for (size_t i = 0; i < 12; i++) {
        const glm::vec3 v1 = (edges_a[i].start - pos_b) * rot_b;
        const glm::vec3 v2 = (edges_a[i].end - pos_b) * rot_b;

        const glm::vec3 transformed_v1 = (edges_a[i].start - pos_a) * rot_a;
        const glm::vec3 transformed_v2 = (edges_a[i].end - pos_a) * rot_a;

        const glm::vec3 diff = v2 - v1;
        const float length = glm::length(diff);

        const float t1 = ray_aabb(-local_half_b, local_half_b, v1, glm::normalize(diff));
        // If ray hit
        if (t1 <= length && t1 > -0.001f) {
            glm::vec3 p = transformed_v1 + glm::normalize(transformed_v2 - transformed_v1) * t1;

            // Check all axes for min and max values
            for (int j = 0; j < 3; j++) {
                if (p[j] < t_min[j]) t_min[j] = p[j];
                if (p[j] > t_max[j]) t_max[j] = p[j];
            }
        }

        const float t2 = ray_aabb(-local_half_b, local_half_b, v2, glm::normalize(v1 - v2));
        // If ray hit
        if (t2 <= length && t2 > -0.001f) {
            glm::vec3 p = transformed_v2 + glm::normalize(transformed_v1 - transformed_v2) * t2;

            // Check all axes for min and max values
            for (int j = 0; j < 3; j++) {
                if (p[j] < t_min[j]) t_min[j] = p[j];
                if (p[j] > t_max[j]) t_max[j] = p[j];
            }
        }
    }

    for (int j = 0; j < 3; j++) {
        min[j] = (int)std::floor((std::clamp(t_min[j] - UNITS_PER_VOXEL, -local_half_a[j], local_half_a[j]) + local_half_a[j]) * VOXELS_PER_UNIT);
        max[j] = (int)std::floor((std::clamp(t_max[j] + UNITS_PER_VOXEL, -local_half_a[j], local_half_a[j]) + local_half_a[j]) * VOXELS_PER_UNIT);
    }
}

float Physics::ray_aabb(const glm::vec3& min, const glm::vec3& max, const glm::vec3& ro, const glm::vec3& rd) const {
    float tmin = 0, tmax = 1e32f;

    /* Loop will be unrolled */
    for (int axis = 0; axis < 3; ++axis) {
        const float t1 = (min[axis] - ro[axis]) / rd[axis];
        const float t2 = (max[axis] - ro[axis]) / rd[axis];

        const float dmin = std::min(t1, t2);
        const float dmax = std::max(t1, t2);

        tmin = std::max(dmin, tmin);
        tmax = std::min(dmax, tmax);
    }

    if (tmax >= tmin) return tmin;
    return 1e32f; /* miss */
}

bool Physics::is_separated(const glm::vec3& axis, const VoxelBody::Box& box_a, const VoxelBody::Box& box_b, float& overlap) const {
    float min_a = 1e32f, max_a = -1e32f;
    float min_b = 1e32f, max_b = -1e32f;

    for (size_t i = 0; i < 8; i++) {
        float projection = glm::dot(box_a.vertex[i], axis);
        min_a = std::min(min_a, projection);
        max_a = std::max(max_a, projection);
    }

    for (size_t i = 0; i < 8; i++) {
        float projection = glm::dot(box_b.vertex[i], axis);
        min_b = std::min(min_b, projection);
        max_b = std::max(max_b, projection);
    }

    if (max_a < min_b || max_b < min_a) return true;

    float max = std::min(max_a, max_b);
    float min = std::max(min_a, min_b);

    // Calculate overlap
    overlap = max - min;

    if (max_b > max_a) overlap = -overlap;

    return false;  // Not separated
}

void Physics::check_neighbors(const VoxelBody& vb_a, const VoxelBody& vb_b, const size_t x, const size_t y, const size_t z, Collision& coll) const {
    const PhysicsVoxel& voxel_a = vb_a.voxels.get_voxel(x, y, z);
    if (voxel_a.type == PhysicsVoxelType::EMPTY) return;

    const glm::vec3 pos_a = vb_a.position;
    const glm::vec3 half_scale_a = glm::vec3(vb_a.width, vb_a.height, vb_a.depth) * 0.5f;
    const glm::quat rot_a = vb_a.rotation;

    const glm::vec3 pos_b = vb_b.position;
    const glm::vec3 half_scale_b = glm::vec3(vb_b.width, vb_b.height, vb_b.depth) * 0.5f;
    const glm::quat rot_b = vb_b.rotation;

    // Transform the voxel from local voxel space A to world space
    glm::vec3 local_pos_a = {x, y, z};
    local_pos_a *= UNITS_PER_VOXEL;
    local_pos_a -= half_scale_a - glm::vec3(VOXEL_SIZE_HALF);
    const glm::vec3 world_pos_a = pos_a + rot_a * local_pos_a;

    // Transform the voxel from world space to local voxel space B
    const glm::vec3 relative_pos = (world_pos_a - pos_b) * rot_b;
    const glm::ivec3 voxel_coord((relative_pos + half_scale_b) * (float)VOXELS_PER_UNIT + glm::vec3(0.5f));

    // Check 2x2x2 area around relative voxel
    bool added_contact = false;
    for (int z_b = -1; z_b < 1; z_b++) {
        for (int y_b = -1; y_b < 1; y_b++) {
            for (int x_b = -1; x_b < 1; x_b++) {
                if (coll.contacts.size() >= max_contacts || added_contact) continue;
                const glm::uvec3 coord(voxel_coord + glm::ivec3(x_b, y_b, z_b));
                // Check if in range
                if (!(coord.x >= 0 && coord.x < vb_b.voxels.get_size().x && coord.y >= 0 && coord.y < vb_b.voxels.get_size().y && coord.z >= 0 && coord.z < vb_b.voxels.get_size().z)) continue;

                const PhysicsVoxel& voxel_b = vb_b.voxels.get_voxel(coord.x, coord.y, coord.z);
                if (voxel_b.type == PhysicsVoxelType::EMPTY) continue;

                if (voxel_a.type != PhysicsVoxelType::CORNER && !(voxel_a.type == PhysicsVoxelType::EDGE && voxel_b.type == PhysicsVoxelType::EDGE)) continue;

                // Transform the voxel from local voxel space B to world space
                glm::vec3 local_pos_b = {coord.x, coord.y, coord.z};
                local_pos_b *= UNITS_PER_VOXEL;
                local_pos_b -= half_scale_b - glm::vec3(VOXEL_SIZE_HALF);
                glm::vec3 world_pos_b = pos_b + rot_b * local_pos_b;

                const glm::vec3 dir(world_pos_b - world_pos_a);
                const float dist = glm::length2(dir);

                // If colliding
                if (dist > VOXEL_SIZE_SQR) continue;

                ContactPoint contact = {};
                contact.point = world_pos_a;

                // Decide penetration
                if (voxel_a.type == PhysicsVoxelType::INSIDE || voxel_b.type == PhysicsVoxelType::INSIDE)
                    contact.penetration = UNITS_PER_VOXEL;
                else
                    contact.penetration = UNITS_PER_VOXEL - sqrtf(dist);

                // Decide normal
                if (voxel_a.type == PhysicsVoxelType::INSIDE || voxel_a.type == PhysicsVoxelType::FACE)
                    contact.normal = glm::normalize((glm::vec3)voxel_a.get_normal() * glm::inverse(rot_a));
                else if (voxel_b.type == PhysicsVoxelType::INSIDE || voxel_b.type == PhysicsVoxelType::FACE)
                    contact.normal = -glm::normalize((glm::vec3)voxel_b.get_normal() * glm::inverse(rot_b));
                else if (voxel_a.normal_index == 0)  // if there is no valid normal index
                    contact.normal = glm::normalize(dir);
                else {
                    // if there is a valid normal index use its value to check validity of normal
                    const glm::vec3 lut_normal = (glm::vec3)voxel_a.get_normal() * glm::inverse(rot_a);
                    const glm::vec3 normal = glm::dot(dir, -lut_normal) < 0 ? glm::normalize(dir) : -glm::normalize(dir);
                    contact.normal = normal;
                }

                if (std::isnan(contact.normal.x)) {
                    continue;
                }

                added_contact = true;
                coll.contacts.push_back(contact);
            }
        }
    }
}

void Physics::apply_velocities() const {
    for (const auto& [vox, vb, transform] : engine.ecs.get_registry().view<VoxelBody, Transform>().each()) {
        if (vb.type == VoxelBody::STATIC) continue;

        vb.position += vb.velocity * Engine::Config::FIXED_TIME_STEP;
        vb.center_of_mass += vb.velocity * Engine::Config::FIXED_TIME_STEP;

        float angle = glm::length(vb.angular_velocity) * Engine::Config::FIXED_TIME_STEP;
        if (angle <= 0.0f) continue;

        glm::vec3 axis = glm::normalize(vb.angular_velocity);
        glm::quat incremental_rot = glm::angleAxis(angle, axis);
        vb.rotation = glm::normalize(incremental_rot * vb.rotation);
    }
}

void Physics::initialize_voxel_body(VoxelBody& vb) {
    // Calculate mass and inertia
    size_t filled_counter = 0;
    glm::vec3 sum = {};
    const glm::vec3 half_scale = glm::vec3(vb.width, vb.height, vb.depth) * 0.5f;

    for (size_t z = 0; z < vb.voxels.get_size().z; z++) {
        for (size_t y = 0; y < vb.voxels.get_size().y; y++) {
            for (size_t x = 0; x < vb.voxels.get_size().x; x++) {
                if (vb.voxels.get_voxel(x, y, z).type == PhysicsVoxelType::EMPTY) continue;

                filled_counter++;
                sum += glm::vec3(x, y, z) * UNITS_PER_VOXEL - half_scale + VOXEL_SIZE_HALF;
            }
        }
    }

    vb.com_local_offset = sum / (float)filled_counter;
    const float voxel_mass = std::powf(UNITS_PER_VOXEL, 3) * vb.density;
    vb.inv_mass = 1.0f / (voxel_mass * filled_counter);

    glm::mat3 inertia_tensor(0.0f);
    for (size_t z = 0; z < vb.voxels.get_size().z; z++) {
        for (size_t y = 0; y < vb.voxels.get_size().y; y++) {
            for (size_t x = 0; x < vb.voxels.get_size().x; x++) {
                if (vb.voxels.get_voxel(x, y, z).type == PhysicsVoxelType::EMPTY) continue;

                const glm::vec3 pos = glm::vec3(x, y, z) * UNITS_PER_VOXEL - half_scale + VOXEL_SIZE_HALF;
                const glm::vec3 r = pos - vb.com_local_offset;
                float r2 = glm::dot(r, r);

                inertia_tensor[0][0] += voxel_mass * (r2 - r.x * r.x);
                inertia_tensor[1][1] += voxel_mass * (r2 - r.y * r.y);
                inertia_tensor[2][2] += voxel_mass * (r2 - r.z * r.z);
                inertia_tensor[0][1] -= voxel_mass * r.x * r.y;
                inertia_tensor[0][2] -= voxel_mass * r.x * r.z;
                inertia_tensor[1][2] -= voxel_mass * r.y * r.z;
            }
        }
    }

    inertia_tensor[1][0] = inertia_tensor[0][1];
    inertia_tensor[2][0] = inertia_tensor[0][2];
    inertia_tensor[2][1] = inertia_tensor[1][2];

    if (inertia_tensor[0][0] == 0) {
        inertia_tensor[0][0] = 1;
    }

    if (inertia_tensor[1][1] == 0) {
        inertia_tensor[1][1] = 1;
    }

    if (inertia_tensor[2][2] == 0) {
        inertia_tensor[2][2] = 1;
    }

    vb.inv_inertia = glm::inverse(inertia_tensor);
}

void Physics::add_force(VoxelBody& vb, const glm::vec3& force) { vb.stored_velocity += force; }

void Physics::add_torque(VoxelBody& vb, const glm::vec3& torque) { vb.stored_torque += torque; }

void Physics::add_force_at_position(VoxelBody& vb, const glm::vec3& force, const glm::vec3& p) {
    const glm::vec3 r = p - vb.position;
    vb.stored_velocity += force * vb.inv_mass;

    // Calculate and apply torque
    const glm::vec3 torque = glm::cross(r, force);
    const glm::mat3 ro = glm::toMat3(vb.rotation);
    const glm::mat3 inv_inertia_world = ro * vb.inv_inertia * glm::transpose(ro);
    vb.stored_torque += inv_inertia_world * torque;
}

}  // namespace tmt
