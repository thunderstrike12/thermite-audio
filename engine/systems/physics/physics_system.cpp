#include "physics_system.hpp"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <queue>

#include "engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/tools/profiler.hpp"

#include "components/voxel_body.hpp"
#include "engine/core/salvo.hpp"

namespace tmt {

void Physics::on_start() {
    Log::info("Physics on_start");
    for (const auto& [entity, vb, transform] : engine.ecs.view<VoxelBody, Transform>().each()) {
        vb.position = transform.get_world_position();
        vb.rotation = transform.get_world_rotation();

        if (vb.type == VoxelBody::DYNAMIC) {
            initialize_voxel_body(vb);
            if (vb.gravity <= 0.0f) {
                vb.type = VoxelBody::SLEEPING;
                vb.accumulated_forces = 0.0f;
            }
        } else {
            glm::uvec3 size = vb.resource->size;
            vb.width = (float)size.x * UNITS_PER_VOXEL;
            vb.height = (float)size.y * UNITS_PER_VOXEL;
            vb.depth = (float)size.z * UNITS_PER_VOXEL;
            vb.center_of_mass = vb.position + (vb.rotation * vb.com_local_offset);
        }
    }
}

void draw_node(tmt::Bvh2<VoxelObject>& bvh, uint32_t current_node) {
    // Draw AABB of the node
    engine.polyline.use_color(0.3f, 0.3f, 1.0f);
    engine.polyline.use_line_width(0.25f);
    engine.polyline.draw_aabb(bvh.nodes[current_node].min_bounds, bvh.nodes[current_node].max_bounds);

    if (bvh.nodes[current_node].is_leaf()) return;
    draw_node(bvh, bvh.nodes[current_node].left_first);
    draw_node(bvh, bvh.nodes[current_node].left_first + 1);
}

void draw_solids(
    tmt::Svt64* tree, uint32_t node_index, uint32_t current_depth, float node_half_extent, const glm::vec3& node_center, const glm::vec3& extents_diff, const glm::quat& rotation
) {
    // Get current node
    const tmt::Svt64Node& node = tree->nodes[node_index];

    // Calculate new half extent
    const float child_half_extent = node_half_extent * 0.25f;

    // Copy mask for iteration
    uint64_t mask = node.child_mask;
    while (mask != 0u) {
        // Find index of first set bit
        const uint32_t i = std::countr_zero(mask);

        // Get local coordinates
        const uint32_t local_x = (i >> 0u) & 3u;
        const uint32_t local_y = (i >> 4u) & 3u;
        const uint32_t local_z = (i >> 2u) & 3u;

        // Calculate local child center
        const glm::vec3 local_offset = glm::vec3(
            (((float)local_x + 0.5f) * 0.25f - 0.5f) * node_half_extent * 2.0f, (((float)local_y + 0.5f) * 0.25f - 0.5f) * node_half_extent * 2.0f,
            (((float)local_z + 0.5f) * 0.25f - 0.5f) * node_half_extent * 2.0f
        );

        // Calculate world position of child
        const glm::vec3 child_center = node_center + (rotation * local_offset);

        // Debug draw OBB
        engine.polyline.use_color(0.1f, 0.9f, 0.1f);
        engine.polyline.use_line_width(0.3f);
        if (current_depth == 1) {
            engine.polyline.use_color(0.7f, 0.9f, 0.1f);
            engine.polyline.use_line_width(0.2f);
        }

        engine.polyline.draw_obb(child_center, glm::vec3(child_half_extent), rotation, Engine::Config::FIXED_TIME_STEP + 0.1f);

        // Get child node index
        const uint32_t child_node_offset = (uint32_t)__popcnt64(node.child_mask & ((1ull << i) - 1u));
        const uint32_t child_node_index = node.abs_ptr() + child_node_offset;

        // Recursively go deeper if child is not a leaf
        if (current_depth + 1 < tree->depth) {
            draw_solids(tree, child_node_index, current_depth + 1, child_half_extent, child_center, extents_diff, rotation);
        }

        // Clear bit
        mask &= ~(1ull << i);
    }
}

void Physics::on_update(const FrameData&) {
    engine.polyline.use_line_width(0.25f);
    for (const auto& [entity, vb, transform] : engine.ecs.view<VoxelBody, Transform>().each()) {
        // auto* tree_a = vb.resource->blas.get();
        // const float half_extent_a = powf(4.0f, (float)tree_a->depth) * 0.5f * UNITS_PER_VOXEL;
        // const glm::vec3 extents_diff_a = half_extent_a - ((glm::vec3)vb.resource->size * 0.5f * UNITS_PER_VOXEL);
        // const glm::vec3 root_center_a = vb.position + (vb.rotation * extents_diff_a);

        // draw_solids(tree_a, 0u, 0u, half_extent_a, root_center_a + glm::vec3(10, 0, 0), extents_diff_a, vb.rotation);

        // auto edges = vb.get_world_edges();
        // engine.polyline.use_color(vb.type == VoxelBody::DYNAMIC ? glm::vec4(0, 1, 0, 1) : glm::vec4(1, 0, 0, 1));
        // for (size_t i = 0; i < edges.size(); i++) {
        //     engine.polyline.draw_line(edges[i].start, edges[i].end);
        // }

        // engine.polyline.use_depth_testing(false);
        // engine.polyline.use_line_width(0.5f);
        // engine.polyline.use_color(1.0f, 0.0f, 0.0f);
        // engine.polyline.draw_circle(vb.center_of_mass, 0.2f);  // glm::vec3(1.0f, 0.0f, 1.0f), 0.2f);
        // engine.polyline.use_color(0.0f, 1.0f, 0.0f);
        // engine.polyline.draw_circle(vb.position, 0.2f);
        // engine.polyline.use_color(0.0f, 0.0f, 1.0f);
        // engine.polyline.draw_circle(vb.position + vb.rotation * vb.com_local_offset, 0.2f);
        /* engine.renderer.draw_cross(vb.position, glm::vec3(1.0f, 0.0f, 0.0f), 0.2f);
         engine.renderer.draw_cross(vb.position + vb.rotation * vb.com_local_offset, glm::vec3(0.0f, 0.0f, 1.0f), 0.2f);*/

        //// Draw voxel normals
        // engine.polyline.use_depth_testing(false);
        // engine.polyline.use_color(1.0f, 0.0f, 0.0f);
        //
        // glm::uvec3 size = vb.resource->size;
        // auto* tree = vb.resource->blas.get();
        // for (size_t z = 0; z < size.z; z++) {
        //     for (size_t y = 0; y < size.y; y++) {
        //         for (size_t x = 0; x < size.x; x++) {
        //             const tmt::PhysicsVoxel* voxel = tree->get_physics_voxel(x, y, z);
        //             if (voxel == nullptr || voxel->normal_index == 0 || voxel->normal_index == 28) continue;

        //            glm::ivec3 local_normal = voxel->get_normal();
        //            glm::vec3 normal = vb.rotation * glm::vec3((float)local_normal.x, (float)local_normal.y, (float)local_normal.z);
        //            glm::vec3 local_pos = glm::vec3(
        //                ((float)x + 0.5f - (size.x * 0.5f)) * UNITS_PER_VOXEL, ((float)y + 0.5f - (size.y * 0.5f)) * UNITS_PER_VOXEL, ((float)z + 0.5f - (size.z * 0.5f)) * UNITS_PER_VOXEL
        //            );
        //            glm::vec3 world_pos = vb.position + (vb.rotation * local_pos);

        //            engine.polyline.draw_line(world_pos, world_pos + normal * 0.05f);
        //            // engine.polyline.draw_arrow(world_pos, normal * 0.1f, 0.15f);
        //            // tmt::engine.renderer.draw_arrow(world_pos, normal, glm::vec3(0.0f, 0.6f, 0.0f), 0.15f, 0.05f);
        //        }
        //    }
        //}
    }

    // if (bvh.nodes) draw_node(bvh, 0u);
    //  draw_node(bvh, bvh.nodes[0].left_first + 1u);
}

void Physics::on_fixed_update(const FrameData&) {
    TMT_ZONE_SCOPED_N("Physics")

    // Update Forces
    for (const auto& [entity, vb, transform] : engine.ecs.view<VoxelBody, Transform>().each()) {
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

        // Accumulate
        float forces = (glm::length2(vb.velocity / Engine::Config::FIXED_TIME_STEP) + glm::length2(vb.angular_velocity / Engine::Config::FIXED_TIME_STEP));
        if (std::isnan(forces)) forces = 0;
        vb.accumulated_forces = vb.accumulated_forces * 0.925f + forces * 0.075f;

        if (vb.type == VoxelBody::SLEEPING) {
            if (vb.accumulated_forces > 150.0f || forces > 150.0f) {
                vb.type = VoxelBody::DYNAMIC;
            } else {
                vb.velocity = glm::vec3(0);
                vb.angular_velocity = glm::vec3(0);
            }
        }

        // practicly not moving
        if (vb.accumulated_forces <= 150.0f && forces <= 150.0f) vb.type = VoxelBody::SLEEPING;
    }

    // Wake up
    engine.salvo.activate_workers();

    // Build physics BVH
    const entt::basic_group group = engine.ecs.group<VoxelBody>(entt::get<Transform>);
    std::vector<VoxelObject> objects {};
    objects.reserve(group.size());
    {
        TMT_ZONE_SCOPED_N("Build BVH")
        for (auto&& [entity, vb, transform] : group.each()) {
            // TODO: THIS CAN CAUSE ISSUES WITH MISSING RESOURCES!!!!

            // if (vb.resource == nullptr) continue;
            /* Convert the entity to a voxel object */
            VoxelObject object {};
            object.local_to_world = transform.get_world_matrix();
            object.world_to_local = glm::inverse(object.local_to_world);
            object.size = vb.resource->size;
            object.mask = (1u << vb.layer);
            object.volume = vb.resource.resource.get();
            objects.push_back(std::move(object));
        }

        bvh.build(objects.data(), (uint32_t)objects.size());
    }

    {
        TMT_ZONE_SCOPED_N("Collision Detection")
        // Detect Colllisions
        // Generate Contacts
        generate_constraints();
    }

    engine.salvo.deactivate_workers();

    {
        TMT_ZONE_SCOPED_N("Solver") {
            TMT_ZONE_SCOPED_N("Solve Velocity")
            // Solve Velocities
            solver.solve_velocities(Engine::Config::FIXED_TIME_STEP);
        }

        {
            TMT_ZONE_SCOPED_N("Apply Velocity")
            // Apply Velocities
            // Update Positions
            apply_velocities();
        }

        {
            TMT_ZONE_SCOPED_N("Solve Position")
            // Solve Positions
            solver.solve_positions(Engine::Config::FIXED_TIME_STEP);
        }
    }
}

void Physics::generate_constraints() {
    // solver.collisions.resize(MAX_CONTACTS);
    const PhysicsGroup group = engine.ecs.group<VoxelBody>(entt::get<Transform>);
    engine.salvo.parallel_for(group.size(), [this, &group](size_t i) { generate_constraint((int)i, group); });
}

void Physics::generate_constraint(int index, const PhysicsGroup& group) {
    Entity entity = group[index];
    VoxelBody& vb = group.get<VoxelBody>(entity);
    // Transform& transform = group.get<Transform>(entity);
    // auto& [vb, transform] = group.get<VoxelBody, Transform>(entity);

    if (vb.type == VoxelBody::STATIC || vb.type == VoxelBody::SLEEPING) return;
    if (vb.resource == nullptr) return;

    Aabb current_aabb = vb.aabb();
    std::vector<uint32_t> hits = bvh.overlap(current_aabb);

    for (uint32_t hit : hits) {
        Entity other_entity = group[hit];
        if (other_entity == entity) continue;

        VoxelBody& other_vb = group.get<VoxelBody>(other_entity);
        // Transform& other_transform = group.get<Transform>(other_entity);
        // auto& [other_vb, other_transform] = group.get<VoxelBody, Transform>(other_entity);

        // --- check layer collision ---
        if (!physics_layers.can_collide(vb.layer, other_vb.layer)) continue;

        if (sat_early_out(vb, other_vb)) continue;

        Collision coll(entity, other_entity);
        coll.contacts.reserve(64);

        auto* tree_a = vb.resource->blas.get();
        const float half_extent_a = powf(4.0f, (float)tree_a->depth) * 0.5f * UNITS_PER_VOXEL;
        const glm::vec3 extents_diff_a = half_extent_a - ((glm::vec3)vb.resource->size * 0.5f * UNITS_PER_VOXEL);
        const glm::vec3 root_center_a = vb.position + (vb.rotation * extents_diff_a);

        auto* tree_b = other_vb.resource->blas.get();
        const float half_extent_b = powf(4.0f, (float)tree_b->depth) * 0.5f * UNITS_PER_VOXEL;
        const glm::vec3 extents_diff_b = half_extent_b - ((glm::vec3)other_vb.resource->size * 0.5f * UNITS_PER_VOXEL);
        const glm::vec3 root_center_b = other_vb.position + (other_vb.rotation * extents_diff_b);

        compare_trees(tree_a, root_center_a, vb.rotation, half_extent_a, tree_b, root_center_b, other_vb.rotation, half_extent_b, coll);

        if (coll.contacts.empty()) continue;

        if (other_vb.type == VoxelBody::SLEEPING) other_vb.accumulated_forces = 10000;

        coll.normal = glm::normalize(other_vb.position - vb.position);
        solver.collisions[solver.contact_index.fetch_add(1)] = coll;
    }
}

void Physics::on_end() {}

void Physics::compare_trees(
    tmt::Svt64* tree_a, const glm::vec3& center_a, const glm::quat& rotation_a, float half_extent_a, tmt::Svt64* tree_b, const glm::vec3& center_b, const glm::quat& rotation_b,
    float half_extent_b, Collision& coll
) {
    TMT_ZONE_SCOPED_N("svt64 cd")
    struct StackEntry {
        uint32_t node_index_a;
        uint32_t node_index_b;
        float half_extent_a;
        float half_extent_b;
        glm::vec3 center_a;
        glm::vec3 center_b;
    };

    std::vector<StackEntry> stack;
    stack.reserve((tree_a->depth + tree_b->depth) * 64);

    // Add root pair
    stack.push_back({ 0, 0, half_extent_a, half_extent_b, center_a, center_b });

    while (!stack.empty()) {
        const StackEntry entry = stack.back();
        stack.pop_back();

        const tmt::Svt64Node& node_a = tree_a->nodes[entry.node_index_a];
        const tmt::Svt64Node& node_b = tree_b->nodes[entry.node_index_b];

        const bool is_leaf_a = node_a.is_leaf();
        const bool is_leaf_b = node_b.is_leaf();

        if (is_leaf_a && is_leaf_b) {
            // Loop over all voxels in leaf A

            // If voxel is overlapping with b leaf and is CORNER or EDGE, Do collision check with all voxels in leaf B

            // Precompute overlap values
            const float child_half_extent_a = entry.half_extent_a * 0.25f;
            const float radius_b = entry.half_extent_b * SQRT3;
            const float combined_radius = child_half_extent_a + radius_b;
            const float combined_radius_sq = combined_radius * combined_radius;

            uint64_t mask_a = node_a.child_mask;
            while (mask_a != 0u) {
                const uint32_t i_a = std::countr_zero(mask_a);
                mask_a &= mask_a - 1u;

                const uint32_t child_offset_a = (uint32_t)__popcnt64(node_a.child_mask & ((1ull << i_a) - 1u));
                const uint32_t child_index_a = node_a.abs_ptr() + child_offset_a;
                const PhysicsVoxel* voxel_a = &tree_a->physics_data[child_index_a];

                if (voxel_a->type == PhysicsVoxelType::INSIDE || voxel_a->type == PhysicsVoxelType::FACE) continue;

                const uint32_t local_x_a = (i_a >> 0u) & 3u;
                const uint32_t local_y_a = (i_a >> 4u) & 3u;
                const uint32_t local_z_a = (i_a >> 2u) & 3u;

                const glm::vec3 local_offset_a = glm::vec3(
                    (((float)local_x_a + 0.5f) * 0.25f - 0.5f) * entry.half_extent_a * 2.0f, (((float)local_y_a + 0.5f) * 0.25f - 0.5f) * entry.half_extent_a * 2.0f,
                    (((float)local_z_a + 0.5f) * 0.25f - 0.5f) * entry.half_extent_a * 2.0f
                );
                const glm::vec3 child_center_a = entry.center_a + (rotation_a * local_offset_a);

                const glm::vec3 diff_a = entry.center_b - child_center_a;
                if (glm::dot(diff_a, diff_a) > combined_radius_sq) {
                    /*engine.polyline.use_color(0.0f, 0.0f, 1.0f, 0.75f);
                    engine.polyline.use_line_width(2.0f, true);
                    engine.polyline.draw_obb(child_center_a, glm::vec3(child_half_extent_a), rotation_a, Engine::Config::FIXED_TIME_STEP + 0.1f);*/

                    continue;
                }

                const float child_half_extent_b = entry.half_extent_b * 0.25f;
                const float combined_child_radius = child_half_extent_a + child_half_extent_b;
                const float child_radius_sq = combined_child_radius * combined_child_radius;

                // Loop over all solid voxels in leaf B
                uint64_t mask_b = node_b.child_mask;
                while (mask_b != 0u) {
                    // Find index of first set bit and clear it
                    const uint32_t i_b = std::countr_zero(mask_b);
                    mask_b &= mask_b - 1u;

                    const uint32_t local_x_b = (i_b >> 0u) & 3u;
                    const uint32_t local_y_b = (i_b >> 4u) & 3u;
                    const uint32_t local_z_b = (i_b >> 2u) & 3u;

                    const glm::vec3 local_offset_b = glm::vec3(
                        (((float)local_x_b + 0.5f) * 0.25f - 0.5f) * entry.half_extent_b * 2.0f, (((float)local_y_b + 0.5f) * 0.25f - 0.5f) * entry.half_extent_b * 2.0f,
                        (((float)local_z_b + 0.5f) * 0.25f - 0.5f) * entry.half_extent_b * 2.0f
                    );
                    const glm::vec3 child_center_b = entry.center_b + (rotation_b * local_offset_b);

                    const glm::vec3 dir = child_center_b - child_center_a;
                    const float dist = glm::dot(dir, dir);
                    if (dist > child_radius_sq) continue;

                    const uint32_t child_offset_b = (uint32_t)__popcnt64(node_b.child_mask & ((1ull << i_b) - 1u));
                    const uint32_t child_index_b = node_b.abs_ptr() + child_offset_b;
                    const PhysicsVoxel* voxel_b = &tree_b->physics_data[child_index_b];

                    ContactPoint contact = {};
                    contact.point = child_center_a;

                    // Decide penetration
                    if (voxel_a->type == PhysicsVoxelType::INSIDE || voxel_b->type == PhysicsVoxelType::INSIDE)
                        contact.penetration = UNITS_PER_VOXEL;
                    else
                        contact.penetration = UNITS_PER_VOXEL - sqrtf(dist);

                    // Decide normal
                    if (voxel_a->type == PhysicsVoxelType::INSIDE || voxel_a->type == PhysicsVoxelType::FACE)
                        contact.normal = glm::normalize((glm::vec3)voxel_a->get_normal() * glm::inverse(rotation_a));
                    else if (voxel_b->type == PhysicsVoxelType::INSIDE || voxel_b->type == PhysicsVoxelType::FACE)
                        contact.normal = -glm::normalize((glm::vec3)voxel_b->get_normal() * glm::inverse(rotation_b));
                    else if (voxel_a->normal_index == 0 || voxel_a->normal_index == INVALID_NORMAL_INDEX)  // if there is no valid normal index
                        contact.normal = glm::normalize(dir);
                    else {
                        // if there is a valid normal index use its value to check validity of normal
                        const glm::vec3 lut_normal = (glm::vec3)voxel_a->get_normal() * glm::inverse(rotation_a);
                        const glm::vec3 normal = glm::dot(dir, -lut_normal) < 0 ? glm::normalize(dir) : -glm::normalize(dir);
                        contact.normal = normal;
                    }

                    if (std::isnan(contact.normal.x)) {
                        continue;
                    }

                    coll.contacts.push_back(contact);

                    /* engine.polyline.use_color(1.0f, 0.0f, 0.0f);
                     engine.polyline.use_line_width(1.0f, true);
                     engine.polyline.draw_obb(child_center_a, glm::vec3(child_half_extent_a), rotation_a, Engine::Config::FIXED_TIME_STEP + 0.1f);
                     engine.polyline.draw_arrow(contact.point, contact.normal, 0.2f, Engine::Config::FIXED_TIME_STEP + 0.1f);*/
                }
            }

            // Debug draw leaf node
            // engine.polyline.use_color(0.0f, 1.0f, 0.0f);
            // engine.polyline.use_line_width(0.1f);
            // engine.polyline.draw_obb(entry.center_a, glm::vec3(entry.half_extent_a), rotation_a, Engine::Config::FIXED_TIME_STEP + 0.1f);
            continue;
        }

        // Go deeper in the tree with larger half-extent
        const bool descend_a = !is_leaf_a && (is_leaf_b || entry.half_extent_a >= entry.half_extent_b);

        if (descend_a) {
            // Precompute overlap values
            const float child_half_extent = entry.half_extent_a * 0.25f;
            const float child_radius_a = child_half_extent * SQRT3;
            const float radius_b = entry.half_extent_b * SQRT3;
            const float combined_radius = child_radius_a + radius_b;
            const float combined_radius_sq = combined_radius * combined_radius;

            uint64_t mask = node_a.child_mask;
            while (mask != 0u) {
                // Find index of first set bit and clear it
                const uint32_t i = std::countr_zero(mask);
                mask &= mask - 1u;

                // Get local coordinates
                const uint32_t local_x = (i >> 0u) & 3u;
                const uint32_t local_y = (i >> 4u) & 3u;
                const uint32_t local_z = (i >> 2u) & 3u;

                // Calculate local child center
                const glm::vec3 local_offset = glm::vec3(
                    (((float)local_x + 0.5f) * 0.25f - 0.5f) * entry.half_extent_a * 2.0f, (((float)local_y + 0.5f) * 0.25f - 0.5f) * entry.half_extent_a * 2.0f,
                    (((float)local_z + 0.5f) * 0.25f - 0.5f) * entry.half_extent_a * 2.0f
                );
                // Calculate world position of child
                const glm::vec3 child_center = entry.center_a + (rotation_a * local_offset);

                // Check overlap before adding
                const glm::vec3 diff = child_center - entry.center_b;
                if (glm::dot(diff, diff) > combined_radius_sq) continue;

                //// Debug draw OBB
                // engine.polyline.use_color(0.1f, 0.1f, 0.9f);
                // engine.polyline.use_line_width(2.0f, true);
                //// engine.polyline.draw_sphere(child_center, child_radius_a, 12u, Engine::Config::FIXED_TIME_STEP + 0.1f);
                // engine.polyline.draw_obb(child_center, glm::vec3(child_half_extent), rotation_a, Engine::Config::FIXED_TIME_STEP + 0.1f);

                // Get child node index
                const uint32_t child_offset = (uint32_t)__popcnt64(node_a.child_mask & ((1ull << i) - 1u));
                const uint32_t child_index = node_a.abs_ptr() + child_offset;

                // Add pair to stack
                stack.push_back({ child_index, entry.node_index_b, child_half_extent, entry.half_extent_b, child_center, entry.center_b });
            }
        } else {
            // Precompute overlap values
            const float child_half_extent = entry.half_extent_b * 0.25f;
            const float radius_a = entry.half_extent_a * SQRT3;
            const float child_radius = child_half_extent * SQRT3;
            const float combined_radius = radius_a + child_radius;
            const float combined_radius_sq = combined_radius * combined_radius;

            uint64_t mask = node_b.child_mask;
            while (mask != 0u) {
                const uint32_t i = std::countr_zero(mask);
                mask &= mask - 1u;

                const uint32_t local_x = (i >> 0u) & 3u;
                const uint32_t local_y = (i >> 4u) & 3u;
                const uint32_t local_z = (i >> 2u) & 3u;

                const glm::vec3 local_offset = glm::vec3(
                    (((float)local_x + 0.5f) * 0.25f - 0.5f) * entry.half_extent_b * 2.0f, (((float)local_y + 0.5f) * 0.25f - 0.5f) * entry.half_extent_b * 2.0f,
                    (((float)local_z + 0.5f) * 0.25f - 0.5f) * entry.half_extent_b * 2.0f
                );
                const glm::vec3 child_center = entry.center_b + (rotation_b * local_offset);

                const glm::vec3 diff = entry.center_a - child_center;
                if (glm::dot(diff, diff) > combined_radius_sq) continue;

                // Debug draw OBB
                /*engine.polyline.use_color(0.1f, 0.9f, 0.1f);
                engine.polyline.use_line_width(0.1f);
                engine.polyline.draw_obb(child_center, glm::vec3(child_half_extent), rotation_b, Engine::Config::FIXED_TIME_STEP + 0.1f);
*/
                const uint32_t child_offset = (uint32_t)__popcnt64(node_b.child_mask & ((1ull << i) - 1u));
                const uint32_t child_index = node_b.abs_ptr() + child_offset;

                stack.push_back({ entry.node_index_a, child_index, entry.half_extent_a, child_half_extent, entry.center_a, child_center });
            }
        }
    }
}

bool Physics::sat_early_out(const VoxelBody& vb_a, const VoxelBody& vb_b) {
    TMT_ZONE_SCOPED_N("SAT")

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

void Physics::apply_velocities() const {
    for (const auto& [entity, vb, transform] : engine.ecs.view<VoxelBody, Transform>().each()) {
        if (vb.type == VoxelBody::STATIC) continue;

        vb.position += vb.velocity * Engine::Config::FIXED_TIME_STEP;
        vb.center_of_mass += vb.velocity * Engine::Config::FIXED_TIME_STEP;

        transform.set_world_position(vb.position);  // TEMP

        float angle = glm::length(vb.angular_velocity) * Engine::Config::FIXED_TIME_STEP;
        if (angle <= 0.0f) {
            transform.set_world_rotation(vb.rotation);
            continue;
        }

        glm::vec3 axis = glm::normalize(vb.angular_velocity);
        glm::quat incremental_rot = glm::angleAxis(angle, axis);
        vb.rotation = glm::normalize(incremental_rot * vb.rotation);

        transform.set_world_rotation(vb.rotation);  // TEMP
    }
}

inline void recurse_mass(
    tmt::Svt64* tree, uint32_t node_index, uint32_t current_depth, float node_half_extent, const glm::vec3& node_center, const glm::vec3& extents_diff, glm::vec3& mass_center_sum,
    uint32_t& voxel_count
) {
    // Get current node
    const tmt::Svt64Node& node = tree->nodes[node_index];

    // Calculate new half extent
    const float child_half_extent = node_half_extent * 0.25f;

    // Copy mask for iteration
    uint64_t mask = node.child_mask;
    while (mask != 0u) {
        // Find index of first set bit
        const uint32_t i = std::countr_zero(mask);

        // Get local coordinates
        const uint32_t local_x = (i >> 0u) & 3u;
        const uint32_t local_y = (i >> 4u) & 3u;
        const uint32_t local_z = (i >> 2u) & 3u;

        // Calculate local child center
        const glm::vec3 local_offset = glm::vec3(
            (((float)local_x + 0.5f) * 0.25f - 0.5f) * node_half_extent * 2.0f, (((float)local_y + 0.5f) * 0.25f - 0.5f) * node_half_extent * 2.0f,
            (((float)local_z + 0.5f) * 0.25f - 0.5f) * node_half_extent * 2.0f
        );

        // Calculate world position of child
        const glm::vec3 child_center = node_center + local_offset;

        // Get child node index
        const uint32_t child_node_offset = (uint32_t)__popcnt64(node.child_mask & ((1ull << i) - 1u));
        const uint32_t child_node_index = node.abs_ptr() + child_node_offset;

        // Recursively go deeper if child is not a leaf
        if (current_depth + 1 < tree->depth) {
            recurse_mass(tree, child_node_index, current_depth + 1, child_half_extent, child_center, extents_diff, mass_center_sum, voxel_count);
        } else {
            voxel_count++;
            mass_center_sum += child_center - extents_diff;
        }

        // Clear bit
        mask &= ~(1ull << i);
    }
}

inline void calculate_center_of_mass(VoxelBody& vb) {
    // Update voxel body dimensions
    const glm::vec3 size = (glm::vec3)vb.resource->size * UNITS_PER_VOXEL;
    vb.width = size.x;
    vb.height = size.y;
    vb.depth = size.z;
    const glm::vec3 half_scale = glm::vec3(vb.width, vb.height, vb.depth) * 0.5f;

    // Calculate half extent
    auto* tree = vb.resource.resource->blas.get();
    const float half_extent = powf(4.0f, (float)tree->depth) * 0.5f * UNITS_PER_VOXEL;
    const glm::vec3 extents_diff = half_extent - ((glm::vec3)vb.resource->size * 0.5f * UNITS_PER_VOXEL);

    // Recurse tree to calculate the center of mass sum and voxel count
    glm::vec3 mass_center_sum = {};
    uint32_t voxel_count = 0;
    const glm::vec3 tree_center = glm::vec3(half_extent);  // Tree is centered here
    recurse_mass(tree, 0, 0, half_extent, tree_center, half_scale - extents_diff, mass_center_sum, voxel_count);

    // Then after getting the result, offset it to your coordinate system:
    vb.com_local_offset = (mass_center_sum / (float)voxel_count) - extents_diff;

    // Apply results to voxel body
    // vb.com_local_offset = mass_center_sum / (float)voxel_count;
    const float voxel_mass = std::powf(UNITS_PER_VOXEL, 3) * vb.density;
    vb.inv_mass = 1.0f / (voxel_mass * voxel_count);
}

inline void recurse_inertia(
    tmt::Svt64* tree, uint32_t node_index, uint32_t current_depth, float node_half_extent, const glm::vec3& node_center, const glm::vec3& extents_diff, const glm::vec3& com_local,
    float voxel_mass, glm::mat3& inertia_tensor
) {
    const tmt::Svt64Node& node = tree->nodes[node_index];
    const float child_half_extent = node_half_extent * 0.25f;

    uint64_t mask = node.child_mask;
    while (mask != 0u) {
        const uint32_t i = std::countr_zero(mask);

        const uint32_t local_x = (i >> 0u) & 3u;
        const uint32_t local_y = (i >> 4u) & 3u;
        const uint32_t local_z = (i >> 2u) & 3u;

        const glm::vec3 local_offset = glm::vec3(
            (((float)local_x + 0.5f) * 0.25f - 0.5f) * node_half_extent * 2.0f, (((float)local_y + 0.5f) * 0.25f - 0.5f) * node_half_extent * 2.0f,
            (((float)local_z + 0.5f) * 0.25f - 0.5f) * node_half_extent * 2.0f
        );

        const glm::vec3 child_center = node_center + local_offset;

        const uint32_t child_node_offset = (uint32_t)__popcnt64(node.child_mask & ((1ull << i) - 1u));
        const uint32_t child_node_index = node.abs_ptr() + child_node_offset;

        if (current_depth + 1 < tree->depth) {
            recurse_inertia(tree, child_node_index, current_depth + 1, child_half_extent, child_center, extents_diff, com_local, voxel_mass, inertia_tensor);
        } else {
            // Leaf node - accumulate inertia contribution
            // Convert to local voxel space (same as mass calculation) then subtract COM
            const glm::vec3 voxel_pos = child_center - extents_diff;
            const glm::vec3 r = voxel_pos - com_local;
            const float r2 = glm::dot(r, r);

            inertia_tensor[0][0] += voxel_mass * (r2 - r.x * r.x);
            inertia_tensor[1][1] += voxel_mass * (r2 - r.y * r.y);
            inertia_tensor[2][2] += voxel_mass * (r2 - r.z * r.z);
            inertia_tensor[0][1] -= voxel_mass * r.x * r.y;
            inertia_tensor[0][2] -= voxel_mass * r.x * r.z;
            inertia_tensor[1][2] -= voxel_mass * r.y * r.z;
        }

        mask &= ~(1ull << i);
    }
}

inline glm::mat3 calculate_inertia_tensor(VoxelBody& vb) {
    auto* tree = vb.resource.resource->blas.get();
    const float half_extent = powf(4.0f, (float)tree->depth) * 0.5f * UNITS_PER_VOXEL;
    const glm::vec3 extents_diff = half_extent - ((glm::vec3)vb.resource->size * 0.5f * UNITS_PER_VOXEL);
    const glm::vec3 tree_center = glm::vec3(half_extent);

    const float voxel_mass = std::powf(UNITS_PER_VOXEL, 3) * vb.density;

    glm::mat3 inertia_tensor(0.0f);
    recurse_inertia(tree, 0, 0, half_extent, tree_center, extents_diff, vb.com_local_offset, voxel_mass, inertia_tensor);

    // Mirror symmetric components
    inertia_tensor[1][0] = inertia_tensor[0][1];
    inertia_tensor[2][0] = inertia_tensor[0][2];
    inertia_tensor[2][1] = inertia_tensor[1][2];

    // Prevent singular matrix
    if (inertia_tensor[0][0] == 0.0f) inertia_tensor[0][0] = 1.0f;
    if (inertia_tensor[1][1] == 0.0f) inertia_tensor[1][1] = 1.0f;
    if (inertia_tensor[2][2] == 0.0f) inertia_tensor[2][2] = 1.0f;

    return inertia_tensor;
}

void Physics::initialize_voxel_body(VoxelBody& vb) {
    TMT_ZONE_SCOPED

    // First calculate mass and COM
    calculate_center_of_mass(vb);

    // Then calculate inertia (needs COM)
    vb.inv_inertia = glm::inverse(calculate_inertia_tensor(vb));

    vb.center_of_mass = vb.position + (vb.rotation * vb.com_local_offset);
}

void Physics::add_force(VoxelBody& vb, const glm::vec3& force) {
    vb.stored_velocity += force;
}

void Physics::add_torque(VoxelBody& vb, const glm::vec3& torque) {
    vb.stored_torque += torque;
}

void Physics::add_force_at_position(VoxelBody& vb, const glm::vec3& force, const glm::vec3& p) {
    const glm::vec3 r = p - vb.position;
    vb.stored_velocity += force * vb.inv_mass;

    // Calculate and apply torque
    const glm::vec3 torque = glm::cross(r, force);
    const glm::mat3 ro = glm::toMat3(vb.rotation);
    const glm::mat3 inv_inertia_world = ro * vb.inv_inertia * glm::transpose(ro);
    vb.stored_torque += inv_inertia_world * torque;
}

void Physics::set_position(VoxelBody& vb, const glm::vec3& position) {
    vb.position = position;
    vb.center_of_mass = vb.position + (vb.rotation * vb.com_local_offset);
}
void Physics::set_rotation(VoxelBody& vb, const glm::quat& rotation) {
    vb.rotation = rotation;
    vb.center_of_mass = vb.position + (vb.rotation * vb.com_local_offset);
}

/**
 * Cast a ray using a layer mask.
 *
 * Example:
 *   uint32_t layer_mask = (1 << 1) | (1 << 2);
 *   Hit hit = physics->raycast(ray, layer_mask);
 *
 * Result:
 *   Layer 0 -> not in mask = cannot hit
 *   Layer 1 -> in mask     = can hit
 *   Layer 2 -> in mask     = can hit
 *   Layer 3 -> not in mask = cannot hit
 *
 * The BVH filters objects internally using bitmask comparison:
 *   (object.mask & layer_mask) != 0
 *
 * Objects whose mask does not overlap with the ray's mask
 * are ignored during traversal.
 */
Hit Physics::raycast(const Ray& ray, uint32_t layer_mask) const {
    return bvh.trace(ray, layer_mask);
}

void Physics::recalculate_physics_data(VoxelBody& vb) {
    initialize_voxel_body(vb);
    recalculate_surface_normals(vb);
}

// Count number of set bits in variable range [0..width]
inline uint32_t popcnt_var64(uint64_t mask, uint32_t width) {
    return (uint32_t)__popcnt64(mask & ((1ull << width) - 1));
}

inline void recurse_recalculate_normals(
    tmt::Svt64* tree, const uint32_t node_index, const uint32_t current_depth, const uint32_t node_scale, const uint32_t global_x, const uint32_t global_y, const uint32_t global_z
) {
    // Get current node
    const tmt::Svt64Node& node = tree->nodes[node_index];

    // Calculate new scale for child nodes
    const uint32_t child_scale = node_scale >> 2u;

    // Copy mask for iteration
    uint64_t mask = node.child_mask;
    while (mask != 0u) {
        // Find index of first set bit
        const uint32_t i = std::countr_zero(mask);

        // Get local coordinates
        const uint32_t local_x = (i >> 0u) & 3u;
        const uint32_t local_y = (i >> 4u) & 3u;
        const uint32_t local_z = (i >> 2u) & 3u;

        // Get child node index
        const uint32_t child_node_offset = (uint32_t)__popcnt64(node.child_mask & ((1ull << i) - 1u));
        const uint32_t child_node_index = node.abs_ptr() + child_node_offset;

        // Recursively go deeper if child is not a leaf
        if (current_depth + 1 < tree->depth) {
            recurse_recalculate_normals(
                tree, child_node_index, current_depth + 1, child_scale, global_x + local_x * child_scale, global_y + local_y * child_scale, global_z + local_z * child_scale
            );
        } else {
            // We are a voxel, calculate normal here
            const uint32_t x = global_x + local_x;
            const uint32_t y = global_y + local_y;
            const uint32_t z = global_z + local_z;

            PhysicsVoxel& voxel = tree->physics_data[child_node_index];
            int empty_sides = 0;
            glm::ivec3 normal = {};

            // -X
            if (tree->is_empty(x - 1, y, z)) {
                normal += glm::ivec3(-1, 0, 0);
                empty_sides++;
            }

            // +X
            if (tree->is_empty(x + 1, y, z)) {
                normal += glm::ivec3(1, 0, 0);
                empty_sides++;
            }

            // -Y
            if (tree->is_empty(x, y - 1, z)) {
                normal += glm::ivec3(0, -1, 0);
                empty_sides++;
            }

            // +Y
            if (tree->is_empty(x, y + 1, z)) {
                normal += glm::ivec3(0, 1, 0);
                empty_sides++;
            }

            // -Z
            if (tree->is_empty(x, y, z - 1)) {
                normal += glm::ivec3(0, 0, -1);
                empty_sides++;
            }

            // +Z
            if (tree->is_empty(x, y, z + 1)) {
                normal += glm::ivec3(0, 0, 1);
                empty_sides++;
            }

            voxel.normal_index = 0;

            // Classify voxel type based on empty neighbors
            if (empty_sides == 0) {
                voxel.type = PhysicsVoxelType::INSIDE;
            } else if (empty_sides == 1) {
                for (size_t i = 1; i < 7; i++) {
                    if (NORMAL_LUT[i] == normal) {
                        voxel.normal_index = i;
                        break;
                    }
                }
                voxel.type = PhysicsVoxelType::FACE;
            } else if (empty_sides == 2) {
                for (size_t i = 7; i < 19; i++) {
                    if (NORMAL_LUT[i] == normal) {
                        voxel.normal_index = i;
                        break;
                    }
                }
                voxel.type = PhysicsVoxelType::EDGE;
            } else {  // empty_sides >= 3
                for (size_t i = 19; i < 27; i++) {
                    if (NORMAL_LUT[i] == normal) {
                        voxel.normal_index = i;
                        break;
                    }
                }
                voxel.type = PhysicsVoxelType::CORNER;
            }
        }

        // Clear bit
        mask &= ~(1ull << i);
    }
}

void Physics::recalculate_surface_normals(VoxelBody& vb) {
    auto* tree = vb.resource.resource->blas.get();
    const uint32_t node_scale = (1u << (tree->depth * 2u));
    recurse_recalculate_normals(tree, 0, 0, node_scale, 0, 0, 0);
}

void Physics::recalculate_surface_normals(VoxelVolume& volume) {
    auto* tree = volume.blas.get();
    const uint32_t node_scale = (1u << (tree->depth * 2u));
    recurse_recalculate_normals(tree, 0, 0, node_scale, 0, 0, 0);
}

}  // namespace tmt
