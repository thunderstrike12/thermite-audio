#include "destruction_system.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/tools/profiler.hpp"
#include <queue>
#include <core/components/voxel_renderer.hpp>
#include "engine/core/resources.hpp"

namespace tmt {

void Destruction::on_start() {}

void Destruction::on_update(const FrameData&) {
    for (const auto& [entity, des, vb, vr] : engine.ecs.view<Destructible, VoxelBody, VoxelRenderer>().each()) {
        if (des.initialized) continue;

        des.initialized = true;
        generate_connection_graph(des, vr.resource->blas.get());
    }

    // for (const auto& [entity, des, vb] : engine.ecs.view<Destructable, VoxelBody>().each()) {
    //     engine.polyline.use_depth_testing(false);
    //     // engine.polyline.use_color(0.1f, 0.3f, 0.9f);
    //     for (DestructionNode& node : des.nodes) {
    //         if (node.level == 10) continue;

    //        const glm::vec3 half_scale = glm::vec3(vb.width, vb.height, vb.depth) * 0.5f;
    //        const uint32_t node_size = (1u << (node.level * 2u));
    //        const glm::vec3 pos = (glm::vec3)(node.global_position + (node_size / 2)) * UNITS_PER_VOXEL + vb.position + 0.05f - half_scale;

    //        // uint32_t voxel_count = vb.resource.resource.get()->blas.get()->voxel_count;
    //        // Log::info("voxels %u", vb.resource.resource.get()->blas.get()->voxel_count);

    //        srand((unsigned int)(node.flood_id << 5u));

    //        if (node.flood_id == 0)
    //            engine.polyline.use_color(0.0f, 0.0f, 0.0f);
    //        else
    //            engine.polyline.use_color(((float)rand() / (float)RAND_MAX), ((float)rand() / (float)RAND_MAX), ((float)rand() / (float)RAND_MAX));

    //        // Draw connections to neighbors
    //        for (size_t i = 0; i < 6; i++) {
    //            for (size_t j = 0; j < node.connections[i].node_indices.size(); j++) {
    //                uint32_t node_index = node.connections[i].node_indices[j];

    //                if (des.nodes[node_index].level == 0) continue;

    //                const uint32_t conn_node_size = (1u << (des.nodes[node_index].level * 2u));
    //                const glm::vec3 connection_pos = (glm::vec3)(des.nodes[node_index].global_position + (conn_node_size / 2)) * UNITS_PER_VOXEL + vb.position + 0.05f - half_scale;

    //                const glm::vec3 p1 = pos * 0.75f + connection_pos * 0.25f;
    //                const glm::vec3 p2 = connection_pos * 0.75f + pos * 0.25f;
    //                // const glm::vec3 diff = connection_pos - pos;
    //                // const glm::vec3 dir = glm::normalize(connection_pos - pos);

    //                // engine.polyline.draw_arrow(p1, dir, glm::length(diff) * 0.5f);
    //                engine.polyline.draw_line(p1, p2);
    //            }
    //        }

    //        engine.polyline.draw_circle(pos, 0.033f * (1 << node.level));

    //        // if (node.level == 0) {
    //        //     engine.polyline.draw_circle(pos, 0.033f);
    //        // } else {
    //        //     engine.polyline.draw_circle(pos, 0.033f * 4.0f);
    //        // }
    //    }
    //}
}

void Destruction::on_fixed_update(const FrameData&) {}

void Destruction::on_end() {}

void Destruction::destroy_voxels(Entity entity, const Stencil* stencil, glm::ivec3 offset) {
    TMT_ZONE_SCOPED_N("Destruction")

    // Get voxel body & destructable components
    VoxelRenderer* vr = engine.ecs.try_get_component<VoxelRenderer>(entity);
    Destructible* des = engine.ecs.try_get_component<Destructible>(entity);
    if (vr == nullptr || des == nullptr) return;

    // Subtract voxels from BLAS
    auto* resource = vr->resource.resource.get();
    resource->blas->subtract(stencil, offset);
    resource->set_dirty();

    {
        TMT_ZONE_SCOPED_N("Clear separation data")

        // Clear separation data
        for (auto& node : des->nodes) {
            if (node.level == 10) continue;

            node.cleared = false;
            node.flood_id = 0ull;
            node.flood_masks.clear();
        }
    }

    // Get all the voxels positions of the empty voxels on the edge of destructed voxels
    std::vector<glm::uvec3> edge_indices;
    fill_edge_indices(edge_indices, entity, stencil, offset);

    // Clear the graph and generate it a new one (to slow)
    // des->clear();
    // generate_connection_graph(*des, resource->blas.get());

    // Regenerate part of the graph based on the edge indices
    // regenerate_connection_graph(*des, resource->blas.get(), stencil, offset);

    des->clear();
    generate_connection_graph(*des, resource->blas.get());

    // Can be done before separating the objects, because the data will be transfered over
    Physics::recalculate_edge_normals(resource->blas.get(), edge_indices);

    // Find where the objects separate and creates new entities for each part
    std::vector<Entity> seperated_entities = find_seperations(entity, edge_indices, *des);

    // Recalculate physics data for all separate voxel bodies
    for (Entity seperate_entity : seperated_entities) {
        if (!engine.ecs.get_registry().valid(seperate_entity)) continue;

        VoxelBody& seperate_vb = engine.ecs.get_component<VoxelBody>(seperate_entity);
        VoxelRenderer& seperate_vr = engine.ecs.get_component<VoxelRenderer>(seperate_entity);
        Physics::initialize_voxel_body(seperate_vb, *seperate_vr.resource.resource.get());
    }
}

uint64_t flood_fill_64(uint64_t seed, uint64_t solid_mask) {
    // Mask to only solid voxels
    uint64_t filled = seed & solid_mask;

    while (true) {
        uint64_t expanded = filled;

        // +X: shift right by 1, mask out voxels that wrapped across X boundary
        expanded |= (filled << 1) & 0xEEEEEEEEEEEEEEEEULL;  // x != 0 mask
        // -X: shift left by 1
        expanded |= (filled >> 1) & 0x7777777777777777ULL;  // x != 3 mask

        // +Z: shift by 4 (z is bits [2:3]), mask out z boundary wraps
        expanded |= (filled << 4) & 0xFFF0FFF0FFF0FFF0ULL;  // z != 0 mask
        // -Z:
        expanded |= (filled >> 4) & 0x0FFF0FFF0FFF0FFFULL;  // z != 3 mask

        // +Y: shift by 16 (y is bits [4:5])
        expanded |= (filled << 16) & 0xFFFFFFFFFFFF0000ULL;  // y != 0 mask
        // -Y:
        expanded |= (filled >> 16) & 0x0000FFFFFFFFFFFFULL;  // y != 3 mask

        // Only keep solid voxels
        expanded &= solid_mask;

        // If nothing changed, we're done
        if (expanded == filled) break;
        filled = expanded;
    }

    return filled;
}

void Destruction::destroy_voxel(Entity entity, glm::uvec3 pos) {
    TMT_ZONE_SCOPED_N("Destruction")

    // Get voxel body & destructable components
    VoxelRenderer* vr = engine.ecs.try_get_component<VoxelRenderer>(entity);
    VoxelBody* vb = engine.ecs.try_get_component<VoxelBody>(entity);
    Destructible* des = engine.ecs.try_get_component<Destructible>(entity);
    if (vr == nullptr || des == nullptr) return;

    // Subtract voxels from BLAS
    auto* resource = vr->resource.resource.get();
    resource->blas->remove_voxel(pos.x, pos.y, pos.z);
    resource->set_dirty();

    // Get neighbors in all cardinal directions
    const glm::ivec3 dirs[] = { { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 } };
    std::vector<glm::uvec3> neighbors;
    neighbors.reserve(6);

    // Get all solid neighbors
    for (int d = 0; d < 6; d++) {
        glm::ivec3 neighbor_pos = (glm::ivec3)pos + dirs[d];

        // Skip if the world position is outside the bounds of the resource,
        if (neighbor_pos.x < 0 || neighbor_pos.y < 0 || neighbor_pos.z < 0) continue;
        if (neighbor_pos.x >= (int)resource->size.x || neighbor_pos.y >= (int)resource->size.y || neighbor_pos.z >= (int)resource->size.z) continue;

        // Skip if the voxel is empty
        if (resource->blas->get_voxel((uint32_t)neighbor_pos.x, (uint32_t)neighbor_pos.y, (uint32_t)neighbor_pos.z) == nullptr) continue;

        neighbors.push_back((glm::uvec3)neighbor_pos);
    }

    // Recalculate the normals around the voxel that has been destroyed
    Physics::recalculate_edge_normals(resource->blas.get(), neighbors);

    // Adjust mass
    // Remove voxel contribution from center of mass
    const float voxel_mass = std::powf(UNITS_PER_VOXEL, 3) * vb->density;
    const float old_mass = vb->inv_mass == 0.0f ? 0.0f : 1.0f / vb->inv_mass;
    const float new_mass = old_mass - voxel_mass;

    // World position of the removed voxels center in local space
    const glm::vec3 voxel_local_pos = ((glm::vec3)pos + 0.5f) * UNITS_PER_VOXEL;

    // Weighted average
    vb->com_local_offset = (vb->com_local_offset * old_mass - voxel_local_pos * voxel_mass) / new_mass;
    vb->inv_mass = 1.0f / new_mass;

    {
        TMT_ZONE_SCOPED_N("Early Out")

        // If we have 0 or 1 neighbor, skip destruction
        if (neighbors.size() <= 1) {
            return;
        }

        // Create solid mask (3x3x3 area of surrounding solid voxels)
        uint64_t solid_mask = 0;
        for (int ly = 0; ly <= 2; ly++) {
            for (int lz = 0; lz <= 2; lz++) {
                for (int lx = 0; lx <= 2; lx++) {
                    // Get neighboring position
                    glm::ivec3 wp = (glm::ivec3)pos + glm::ivec3(lx - 1, ly - 1, lz - 1);

                    // Skip voxel that we just destroyed
                    if (lx == 1 && ly == 1 && lz == 1) continue;

                    // Bounds check
                    if (wp.x < 0 || wp.y < 0 || wp.z < 0) continue;
                    if (wp.x >= (int)resource->size.x || wp.y >= (int)resource->size.y || wp.z >= (int)resource->size.z) continue;

                    // Add to the mask if its solid
                    if (resource->blas->get_voxel((uint32_t)wp.x, (uint32_t)wp.y, (uint32_t)wp.z) != nullptr) solid_mask |= 1ULL << (lx + lz * 4 + ly * 16);
                }
            }
        }

        // Get index of neighbor 0 as a mask
        glm::ivec3 s = (glm::ivec3)neighbors[0] - (glm::ivec3)pos + glm::ivec3(1, 1, 1);
        uint64_t seed = 1ULL << (s.x + s.z * 4 + s.y * 16);

        // Do a floodfill on 4^3 grid of bits
        uint64_t fill = flood_fill_64(seed, solid_mask);

        // If the solid_mask != the fill, some parts are unreachable
        if (fill == solid_mask) return;
    }

    // NOTE: Replace this with custom recalculate graph function for a single voxel
    // Clear the graph and generate it a new one (to slow)
    des->clear();
    generate_connection_graph(*des, resource->blas.get());

    // Find where the objects separate and creates new entities for each part
    std::vector<Entity> seperated_entities = find_seperations(entity, neighbors, *des);

    // Recalculate physics data for all separate voxel bodies
    for (Entity seperate_entity : seperated_entities) {
        if (!engine.ecs.get_registry().valid(seperate_entity)) continue;

        VoxelBody& seperate_vb = engine.ecs.get_component<VoxelBody>(seperate_entity);
        VoxelRenderer& seperate_vr = engine.ecs.get_component<VoxelRenderer>(seperate_entity);
        Physics::initialize_voxel_body(seperate_vb, *seperate_vr.resource.resource.get());
    }
}

void Destruction::fill_edge_indices(std::vector<glm::uvec3>& edge_indices, Entity entity, const Stencil* stencil, glm::ivec3 offset) {
    TMT_ZONE_SCOPED

    VoxelRenderer& vr = engine.ecs.get_component<VoxelRenderer>(entity);
    auto* resource = vr.resource.resource.get();
    Svt64* tree = resource->blas.get();

    const glm::ivec3 dirs[] = { { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 } };

    // Loop over the stencil and find all voxels where destruction happened next to
    for (uint32_t local_z = 0; local_z < stencil->size.z; local_z++) {
        for (uint32_t local_y = 0; local_y < stencil->size.y; local_y++) {
            for (uint32_t local_x = 0; local_x < stencil->size.x; local_x++) {
                uint32_t i = local_x + local_y * stencil->size.x + local_z * stencil->size.x * stencil->size.y;

                // if this voxel is empty, skip it
                if (stencil->data[i] == 0) continue;
                glm::ivec3 tree_pos = glm::ivec3(local_x, local_y, local_z) + offset;

                // If the world position of this voxel is outside the bounds of the resource, skip it
                if (tree_pos.x < 0 || tree_pos.y < 0 || tree_pos.z < 0 || tree_pos.x > (int)resource->size.x - 1 || tree_pos.y > (int)resource->size.y - 1 ||
                    tree_pos.z > (int)resource->size.z - 1)
                    continue;

                const uint32_t edge_vals[] = { 0, stencil->size.x - 1, 0, stencil->size.y - 1, 0, stencil->size.z - 1 };
                const glm::ivec3 local_pos(local_x, local_y, local_z);

                for (int d = 0; d < 6; d++) {
                    // If we are on the edge of the stencil, we can directly mark the neighboring voxel
                    uint32_t local_axis_val = local_pos[d / 2];
                    if (local_axis_val == edge_vals[d]) {
                        glm::ivec3 neighbor_pos = local_pos + dirs[d] + offset;

                        // If the world position is outside the bounds of the resource, skip it
                        if (neighbor_pos.x < 0 || neighbor_pos.y < 0 || neighbor_pos.z < 0) continue;
                        if (neighbor_pos.x >= (int)resource->size.x || neighbor_pos.y >= (int)resource->size.y || neighbor_pos.z >= (int)resource->size.z) continue;

                        // If this voxel is empty in the tree, skip it
                        if (tree->get_physics_voxel(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z) == nullptr) continue;

                        edge_indices.push_back((glm::uvec3)neighbor_pos);
                    } else {  // We are not on the edge of the stencil so only mark the neighboring voxel if its empty int the tree
                        glm::ivec3 neighbor_pos = local_pos + dirs[d] + offset;

                        // If the world position is outside the bounds of the resource, skip it
                        if (neighbor_pos.x < 0 || neighbor_pos.y < 0 || neighbor_pos.z < 0) continue;
                        if (neighbor_pos.x >= (int)resource->size.x || neighbor_pos.y >= (int)resource->size.y || neighbor_pos.z >= (int)resource->size.z) continue;

                        // If this voxel is empty in the tree, skip it
                        if (tree->get_physics_voxel(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z) == nullptr) continue;

                        edge_indices.push_back((glm::uvec3)neighbor_pos);
                    }
                }
            }
        }
    }
}

// -X, +X, -Y, +Y, -Z, +Z
constexpr glm::ivec3 dirs[6] = { { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 } };

constexpr uint64_t edge_masks[6] {
    0x1111111111111111ULL,  // -X
    0x8888888888888888ULL,  // +X
    0x000000000000FFFFULL,  // -Y
    0xFFFF000000000000ULL,  // +Y
    0x000F000F000F000FULL,  // -Z
    0xF000F000F000F000ULL   // +Z
};

Svt64Node* get_tree_node_and_mark(uint32_t x, uint32_t y, uint32_t z, Svt64* tree, Destructible& graph, std::vector<uint64_t>& tree_masks, uint64_t mask) {
    TMT_ZONE_SCOPED

    tmt::Svt64Node* node = &tree->nodes[0];
    tree_masks[0] |= mask;
    glm::uvec3 global_pos = glm::uvec3(0);
    // Find the node relavent to that index
    for (uint32_t level = 1u; level < tree->depth; ++level) {
        // Get position of neighbor on this level
        const uint32_t x_index = (x >> ((tree->depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((tree->depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((tree->depth - level) * 2u)) & 3u;

        // Get node index of this neighbor
        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

        // If no node exists on this position and we are at a higher level (should never happen)
        if ((node->child_mask & (1ull << child_index)) == 0u) break;

        const uint32_t child_pos = (uint32_t)__popcnt64(node->child_mask & ((1ull << child_index) - 1u));
        const uint32_t neighbor_node_index = node->abs_ptr() + child_pos;

        const uint16_t level_scale = 1u << ((tree->depth - level) * 2u);

        global_pos += glm::uvec3(x_index * level_scale, y_index * level_scale, z_index * level_scale);

        // Check if connection node exists for this depth on the edge position
        DestructionNode* des_node = graph.get_node(global_pos.x, global_pos.y, global_pos.z);
        if (des_node != nullptr && des_node->level == tree->depth - level) {
            tree_masks[neighbor_node_index] |= mask;
            return &tree->nodes[neighbor_node_index];
        }

        tree_masks[neighbor_node_index] |= mask;
        node = &tree->nodes[neighbor_node_index];
    }

    return node;
}

Svt64Node* get_tree_node_at_position(uint32_t x, uint32_t y, uint32_t z, Svt64* tree, Destructible& graph) {
    TMT_ZONE_SCOPED

    tmt::Svt64Node* node = &tree->nodes[0];
    glm::uvec3 global_pos = glm::uvec3(0);
    // Find the node relavent to that index
    for (uint32_t level = 1u; level < tree->depth; ++level) {
        // Get position of neighbor on this level
        const uint32_t x_index = (x >> ((tree->depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((tree->depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((tree->depth - level) * 2u)) & 3u;

        // Get node index of this neighbor
        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

        // If no node exists on this position and we are at a higher level (should never happen)
        if ((node->child_mask & (1ull << child_index)) == 0u) break;

        const uint32_t child_pos = (uint32_t)__popcnt64(node->child_mask & ((1ull << child_index) - 1u));
        const uint32_t neighbor_node_index = node->abs_ptr() + child_pos;

        const uint16_t level_scale = 1u << ((tree->depth - level) * 2u);

        global_pos += glm::uvec3(x_index * level_scale, y_index * level_scale, z_index * level_scale);

        // Check if connection node exists for this depth on the edge position
        DestructionNode* des_node = graph.get_node(global_pos.x, global_pos.y, global_pos.z);
        if (des_node != nullptr && des_node->level == tree->depth - level) {
            return &tree->nodes[neighbor_node_index];
        }

        node = &tree->nodes[neighbor_node_index];
    }

    return node;
}

// Forward declare function
// void separation_flood(Destructable& graph, DestructionNode& current_node, uint8_t id, Svt64* tree);
// , std::vector<Destruction::FloodStackEntry>& stack

void low_level_separation_flood(
    Destructible& graph, DestructionNode& current_node, uint8_t id, uint64_t overlap_mask, Svt64* tree, std::vector<Destruction::FloodStackEntry>& stack, std::vector<uint64_t>& tree_masks
) {
    // Get svt64 node
    const Svt64Node& tree_node = *get_tree_node_at_position(current_node.global_position.x, current_node.global_position.y, current_node.global_position.z, tree, graph);

    // If there are no solid voxels on the overlap, return
    if ((tree_node.child_mask & overlap_mask) == 0ull) return;

    current_node.flood_id |= (1ull << id);
    get_tree_node_and_mark(current_node.global_position.x, current_node.global_position.y, current_node.global_position.z, tree, graph, tree_masks, current_node.flood_id);

    uint64_t flood_mask = 0u;

    {
        TMT_ZONE_SCOPED_N("flood of low level")

        while (overlap_mask != 0ull) {
            const uint32_t i = std::countr_zero(overlap_mask);

            uint64_t seed = 1ull << i;

            // Skip if already flooded
            if ((flood_mask & seed) != 0ull) {
                overlap_mask &= ~seed;
                continue;
            }

            uint64_t region = flood_fill_64(seed, tree_node.child_mask & ~flood_mask);
            flood_mask |= region;

            overlap_mask &= ~(1ull << i);
        }
    }

    current_node.flood_masks.push_back({ id, flood_mask });

    // Add up all flood masks
    uint64_t all_masks = 0ull;
    for (const auto& filled : current_node.flood_masks) {
        if (filled.first != id) continue;
        all_masks |= filled.second;
    }

    // When flood only touched part of the mask
    if (all_masks == tree_node.child_mask) current_node.cleared = true;

    // Loop over connections
    // For all directions
    for (size_t i = 0; i < 6; i++) {
        // If there is no overlap with the on the relavant axis, skip this direction, or if there are no nodes in this direction
        if ((flood_mask & edge_masks[i]) == 0u || current_node.connections[i].node_indices.empty()) continue;

        // For all connections in this direction (can only be per axis 1 on the lowest level)
        // for (size_t j = 0; j < current_node.connections[i].node_indices.size(); j++) {

        const uint32_t connection_index = current_node.connections[i].node_indices[0];
        DestructionNode& connected_node = graph.nodes[connection_index];

        if (connected_node.cleared) continue;

        // If the node is not a leaf node in the tree, or is fully filled
        if (connected_node.level != 1 && (connected_node.flood_id & (1ull << id)) == 0) {
            // separation_flood(graph, connected_node, id, tree);
            stack.push_back({ connected_node.id, 0ull });
            continue;
        }

        // If the neighboring node is level 1, do low_level_separation_flood
        // If the neighboring node is not level 1, do separation_flood

        // Flip the mask on the defined axis
        uint64_t edge_mask = flood_mask;
        const uint8_t axis = (uint8_t)i / 2;
        // VIBE CODED
        if (axis == 0) {         // X axis
            edge_mask = ((edge_mask & 0x1111111111111111ULL) << 3) | ((edge_mask & 0x2222222222222222ULL) << 1) | ((edge_mask & 0x4444444444444444ULL) >> 1) |
                        ((edge_mask & 0x8888888888888888ULL) >> 3);
        } else if (axis == 1) {  // Y axis
            edge_mask = ((edge_mask & 0x000000000000FFFFULL) << 48) | ((edge_mask & 0x00000000FFFF0000ULL) << 16) | ((edge_mask & 0x0000FFFF00000000ULL) >> 16) |
                        ((edge_mask & 0xFFFF000000000000ULL) >> 48);
        } else if (axis == 2) {  // Z axis
            edge_mask = ((edge_mask & 0x000F000F000F000FULL) << 12) | ((edge_mask & 0x00F000F000F000F0ULL) << 4) | ((edge_mask & 0x0F000F000F000F00ULL) >> 4) |
                        ((edge_mask & 0xF000F000F000F000ULL) >> 12);
        }

        // Mask the edge (take opposite edge because we flipped the mask)
        edge_mask &= edge_masks[i ^ 1u];

        // Remove parts that are already on the others mask
        uint64_t mask_accum = 0ull;
        for (const auto& filled : connected_node.flood_masks) {
            if (filled.first != id) continue;
            mask_accum |= filled.second;
        }

        edge_mask &= ~mask_accum;

        if (edge_mask == 0ull) continue;

        // Get the mask of the svt64 node, flip it, apply edge mask, give it to
        // low_level_separation_flood(graph, connected_node, id, edge_mask, tree);
        stack.push_back({ connected_node.id, edge_mask });
    }
    //}
}

void separation_flood(Destructible& graph, DestructionNode& current_node, uint8_t id, Svt64* tree, std::vector<Destruction::FloodStackEntry>& stack, std::vector<uint64_t>& tree_masks) {
    current_node.flood_id |= (1ull << id);
    current_node.cleared = true;

    get_tree_node_and_mark(current_node.global_position.x, current_node.global_position.y, current_node.global_position.z, tree, graph, tree_masks, current_node.flood_id);

    // For all directions
    for (size_t i = 0; i < 6; i++) {
        // For all connections in this direction
        for (size_t j = 0; j < current_node.connections[i].node_indices.size(); j++) {
            const uint32_t connection_index = current_node.connections[i].node_indices[j];
            DestructionNode& connected_node = graph.nodes[connection_index];
            // If we visited this node before, continue
            if ((connected_node.flood_id & (1ull << id)) != 0) continue;

            // Pray that compiler optimizes this in the if statement below
            const Svt64Node& node_neighbor = *get_tree_node_at_position(connected_node.global_position.x, connected_node.global_position.y, connected_node.global_position.z, tree, graph);

            // If the node is not a leaf node in the tree, or is fully filled
            if (connected_node.level != 1 || node_neighbor.child_mask == ~(0ull)) {
                // separation_flood(graph, connected_node, id, tree);
                stack.push_back({ connected_node.id, 0u });
                continue;
            }

            // Flip the mask on the defined axis
            uint64_t edge_mask = node_neighbor.child_mask;
            const uint8_t axis = (uint8_t)i / 2;
            if (axis == 0) {         // X axis
                edge_mask = ((edge_mask & 0x1111111111111111ULL) << 3) | ((edge_mask & 0x2222222222222222ULL) << 1) | ((edge_mask & 0x4444444444444444ULL) >> 1) |
                            ((edge_mask & 0x8888888888888888ULL) >> 3);
            } else if (axis == 1) {  // Y axis
                edge_mask = ((edge_mask & 0x000000000000FFFFULL) << 48) | ((edge_mask & 0x00000000FFFF0000ULL) << 16) | ((edge_mask & 0x0000FFFF00000000ULL) >> 16) |
                            ((edge_mask & 0xFFFF000000000000ULL) >> 48);
            } else if (axis == 2) {  // Z axis
                edge_mask = ((edge_mask & 0x000F000F000F000FULL) << 12) | ((edge_mask & 0x00F000F000F000F0ULL) << 4) | ((edge_mask & 0x0F000F000F000F00ULL) >> 4) |
                            ((edge_mask & 0xF000F000F000F000ULL) >> 12);
            }

            // Mask the edge (NOTE: CHECK IF THIS IS CORRECT!)
            edge_mask &= edge_masks[i ^ 1u];

            if (edge_mask == 0ull) continue;

            // Get the mask of the svt64 node, flip it, apply edge mask, give it to
            // low_level_separation_flood(graph, connected_node, id, edge_mask, tree);
            stack.push_back({ connected_node.id, edge_mask });
        }
    }
}

void shrink_tree(Svt64* tree, Svt64Node& current_node, glm::uvec3& offset) {
    // Replace root node
    tree->nodes[0] = current_node;
    tree->depth--;

    if ((uint32_t)__popcnt64(current_node.child_mask) == 1u && !current_node.is_leaf()) {
        // Get the single child's bit index
        const uint32_t bit = std::countr_zero(current_node.child_mask);

        // Get local position from bit index
        const uint32_t local_x = (bit >> 0u) & 3u;
        const uint32_t local_z = (bit >> 2u) & 3u;
        const uint32_t local_y = (bit >> 4u) & 3u;

        // Scale is the size of one child at the new depth
        const uint32_t child_scale = 1u << ((tree->depth - 1u) * 2u);

        offset += glm::uvec3(local_x, local_y, local_z) * child_scale;

        // Recurse into first (and only) node
        shrink_tree(tree, tree->nodes[current_node.abs_ptr()], offset);
    }
}

void fill_volumes(const Svt64* original_tree, const std::vector<uint64_t>& tree_masks, std::vector<Entity>& entities, const Destructible& graph) {
    for (size_t i = 0; i < entities.size(); i++) {
        VoxelRenderer& vr = engine.ecs.get_component<VoxelRenderer>(entities[i]);
        VoxelBody& vb = engine.ecs.get_component<VoxelBody>(entities[i]);
        Transform& transform = engine.ecs.get_component<Transform>(entities[i]);
        Svt64* tree = vr.resource.resource->blas.get();

        /* Allocate space for new tree */
        tree->depth = original_tree->depth;
        tree->nodes = new Svt64Node[original_tree->node_count];
        tree->node_count = 1u;
        // TODO: Figure out exact material count
        tree->materials = new MaterialIndex[original_tree->voxel_count];
        tree->physics_data = new PhysicsVoxel[original_tree->voxel_count];
        tree->voxels_capacity = original_tree->voxel_count;
        tree->nodes_capacity = original_tree->node_count;
        tree->voxel_count = 0u;
        tree->palette = original_tree->palette;

        // Build new tree from the tree_masks
        tree->nodes[0] = tree->subdivide_masked(tree->depth * 2u, glm::uvec3(0u), original_tree, 0u, tree_masks, (uint8_t)i, graph);

        // Shrink tree
        // Offset initial depth
        glm::uvec3 tree_offset = glm::uvec3(0);
        tree->depth++;
        shrink_tree(tree, tree->nodes[0], tree_offset);

        // Get max bounds
        glm::uvec3 max = tree->get_max() + glm::uvec3(1);

        // Transform position of new object
        glm::uvec3 diff = vr.resource->size - max;
        glm::vec3 offset = ((glm::vec3)vr.resource->size - (glm::vec3)max) * UNITS_PER_VOXEL * 0.5f;
        glm::vec3 rotated_pos = vb.rotation * ((glm::vec3)tree_offset * UNITS_PER_VOXEL - offset);

        vb.position += rotated_pos;

        transform.set_world_position(vb.position);
        transform.set_world_rotation(vb.rotation);
        // vb.position += rotated_pos;  //((glm::vec3)offset - (glm::vec3)diff) * 0.5f * UNITS_PER_VOXEL;

        // Update tree size
        vr.resource->size = max;

        vr.resource->set_dirty();
    }

    for (size_t i = 0; i < entities.size(); i++) {
        VoxelRenderer& vr = engine.ecs.get_component<VoxelRenderer>(entities[i]);
        Svt64* tree = vr.resource.resource->blas.get();
        // Destroy very small objects
        if (tree->voxel_count < 4) {
            engine.ecs.disable(entities[i], true);
            // engine.ecs.destroy_entity(entities[i]);
            continue;
        }
    }
}

std::vector<Entity> Destruction::find_seperations(Entity entity, const std::vector<glm::uvec3>& edge_indices, Destructible& graph) {
    TMT_ZONE_SCOPED

    VoxelRenderer& vr = engine.ecs.get_component<VoxelRenderer>(entity);
    VoxelBody& vb = engine.ecs.get_component<VoxelBody>(entity);
    auto* resource = vr.resource.resource.get();
    Svt64* tree = resource->blas.get();
    size_t max_depth = 0;
    uint8_t flood_id = 0;

    std::vector<uint64_t> tree_masks;
    tree_masks.resize(tree->node_count);

    // Loop over edge indices
    for (const glm::uvec3& edge_pos : edge_indices) {
        DestructionNode* current_node = nullptr;

        tmt::Svt64Node* node = &tree->nodes[0];
        glm::uvec3 global_pos = glm::uvec3(0);
        // Find the node relavent to that index
        for (uint32_t level = 1u; level < tree->depth; ++level) {
            // Get position of neighbor on this level
            const uint32_t x_index = (edge_pos.x >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t y_index = (edge_pos.y >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t z_index = (edge_pos.z >> ((tree->depth - level) * 2u)) & 3u;

            // Get node index of this neighbor
            const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

            // If no node exists on this position and we are at a higher level (should never happen)
            if ((node->child_mask & (1ull << child_index)) == 0u) break;

            const uint32_t child_pos = (uint32_t)__popcnt64(node->child_mask & ((1ull << child_index) - 1u));
            const uint32_t neighbor_node_index = node->abs_ptr() + child_pos;

            const uint16_t level_scale = 1u << ((tree->depth - level) * 2u);

            global_pos += glm::uvec3(x_index * level_scale, y_index * level_scale, z_index * level_scale);

            // Check if connection node exists for this depth on the edge position
            DestructionNode* des_node = graph.get_node(global_pos.x, global_pos.y, global_pos.z);
            if (des_node != nullptr && des_node->level == tree->depth - level) {
                node = &tree->nodes[neighbor_node_index];
                current_node = des_node;
                break;
            }

            node = &tree->nodes[neighbor_node_index];
        }

        // If we coulnd't find a node, or it has already been touched by the floodfill
        if (current_node == nullptr || current_node->cleared || (current_node->flood_id & (1ull << flood_id)) != 0) continue;

        // Start flood fill without recursive functions
        std::vector<FloodStackEntry> stack;
        stack.reserve(graph.node_count);

        // Add first stack entry
        if (current_node->level != 1 || node->child_mask == ~(0ull)) {
            // Start floodfill from that node
            separation_flood(graph, *current_node, flood_id, tree, stack, tree_masks);
            flood_id++;
        } else {
            // Get position and index
            const uint32_t x_index = (edge_pos.x) & 3u;
            const uint32_t y_index = (edge_pos.y) & 3u;
            const uint32_t z_index = (edge_pos.z) & 3u;
            const uint32_t index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

            // Make mask from the index
            const uint64_t mask = 1ull << index;

            // Find out if the voxel has already been flooded by any other ID
            bool has_flooded = false;
            for (const auto& filled : current_node->flood_masks) {
                const uint64_t filled_mask = filled.second;
                if ((filled_mask & mask) != 0ll) {
                    has_flooded = true;
                    break;
                }
            }

            // Skip the floodfill if this voxel has already been flooded
            if (!has_flooded) {
                low_level_separation_flood(graph, *current_node, flood_id, mask, tree, stack, tree_masks);
                flood_id++;
            } else {
                continue;
            }
        }

        while (!stack.empty()) {
            if (stack.size() > max_depth) max_depth = stack.size();

            // First in last out (depth first)
            FloodStackEntry stack_entry = stack.back();
            stack.pop_back();

            if (graph.nodes[stack_entry.node_index].cleared) continue;

            if (stack_entry.edge_mask != 0) {
                // Bit level floodfill
                low_level_separation_flood(graph, graph.nodes[stack_entry.node_index], flood_id - 1u, stack_entry.edge_mask, tree, stack, tree_masks);
            } else {
                // Graph level floodfill
                separation_flood(graph, graph.nodes[stack_entry.node_index], flood_id - 1u, tree, stack, tree_masks);
            }
        }

        // flood_id++;
    }

    // num of trees = flood_id (return original entity if we only found 1 object)
    if (flood_id <= 1) return std::vector<Entity> { entity };

    std::vector<Entity> entities;
    entities.resize(flood_id);

    std::string original_name = engine.ecs.get_component<Name>(entity).name;

    for (size_t i = 0; i < entities.size(); i++) {
        entities[i] = engine.ecs.create_entity(std::format("{} part {}", original_name, (int)i));
        // engine.ecs.add_component<VoxelRenderer>(entities[i]);
        // engine.ecs.add_component<VoxelBody>(entities[i]);

        VoxelRenderer& new_vr = engine.ecs.add_component<VoxelRenderer>(entities[i]);
        new_vr.resource = { {}, std::make_shared<VoxelVolume>() };
        new_vr.resource.resource->size = vr.resource->size;

        // Transform& new_transform = engine.ecs.add_component<Transform>(entities[i]);

        engine.ecs.add_component<Destructible>(entities[i]);
        VoxelBody& new_vb = engine.ecs.add_component<VoxelBody>(entities[i]);
        new_vb.position = vb.position;
        new_vb.rotation = vb.rotation;
        new_vb.gravity = 0.0f;
        // if (i == 1) vb.gravity = 9.0f;
        new_vb.type = VoxelBody::DYNAMIC;
        // vr.resource = { {}, std::make_shared<tmt::VoxelVolume>() };
    }

    fill_volumes(tree, tree_masks, entities, graph);

    engine.ecs.destroy_entity(entity);

    return entities;
}

bool all_children_solid(Svt64* tree, const uint32_t node_index) {
    // Get current node
    const tmt::Svt64Node& node = tree->nodes[node_index];

    // Copy mask for iteration
    uint64_t mask = node.child_mask;

    // Return false if we are not fully solid
    if (mask != 0xFFFFFFFFFFFFFFFF) return false;

    // if we are a leaf node, and we are fully solid return true
    if (node.is_leaf()) return true;

    // Recurse into all chidren
    for (uint32_t i = 0; i < 64; i++) {
        if (!all_children_solid(tree, node.abs_ptr() + i)) return false;
    }

    return true;
}

void set_lower_level_neighbors(
    Destructible& graph, DestructionNode& connection_node, Svt64* tree, const uint32_t node_index, const uint32_t current_depth, const uint32_t node_scale, const uint32_t global_x,
    const uint32_t global_y, const uint32_t global_z, uint64_t edge_mask, uint8_t from_dir, uint8_t to_dir
) {
    // Get current node
    const tmt::Svt64Node& node = tree->nodes[node_index];

    // Loop over all solid children on the edge
    uint64_t mask = edge_mask & node.child_mask;
    while (mask != 0u) {
        // Find index of first set bit
        const uint32_t i = std::countr_zero(mask);

        // Get global coordinates
        const uint32_t x = global_x + ((i >> 0u) & 3u) * node_scale;
        const uint32_t y = global_y + ((i >> 4u) & 3u) * node_scale;
        const uint32_t z = global_z + ((i >> 2u) & 3u) * node_scale;

        // Get child node index
        const uint32_t child_node_offset = (uint32_t)__popcnt64(node.child_mask & ((1ull << i) - 1u));
        const uint32_t child_node_index = node.abs_ptr() + child_node_offset;

        // If there is a connection node on the neighbors position, make connection
        DestructionNode* neighbor = graph.get_node(x, y, z);
        if (neighbor != nullptr && neighbor->level == tree->depth - current_depth) {
            // Make connections both ways (check on wich axis)
            connection_node.connections[from_dir].add_connection(neighbor->id);
            neighbor->connections[to_dir].add_connection(connection_node.id);
        } else if (current_depth > 1) {  // If this child is not a leaf node, go one level deeper
            set_lower_level_neighbors(graph, connection_node, tree, child_node_index, current_depth - 1u, node_scale >> 2u, x, y, z, edge_mask, from_dir, to_dir);
        }

        // Unset this bit from the mask
        mask &= ~(1ull << i);
    }
}

void set_highest_level_neighbors(
    Destructible& graph, DestructionNode& connection_node, Svt64* tree, const uint32_t node_scale, const uint32_t global_x, const uint32_t global_y, const uint32_t global_z
) {
    {  // -X
        tmt::Svt64Node* node = &tree->nodes[0];
        glm::uvec3 global_pos = glm::uvec3(0u);
        // Skip the deepest part of the tree (voxels)
        for (uint32_t level = 1u; level < tree->depth; ++level) {
            if (node_scale > global_x) break;

            // Get position of neighbor on this level
            const uint32_t x_index = ((global_x - node_scale) >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t y_index = (global_y >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t z_index = (global_z >> ((tree->depth - level) * 2u)) & 3u;

            // Get node index of this neighbor
            const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

            // If no node exists on this position and we are at a higher level
            if ((node->child_mask & (1ull << child_index)) == 0u) break;

            const uint32_t child_pos = (uint32_t)__popcnt64(node->child_mask & ((1ull << child_index) - 1u));
            const uint32_t neighbor_node_index = node->abs_ptr() + child_pos;

            const uint16_t level_scale = 1u << ((tree->depth - level) * 2u);

            global_pos += glm::uvec3(x_index * level_scale, y_index * level_scale, z_index * level_scale);

            // Check if connection node exists for this depth on the neighbors position
            DestructionNode* neighbor = graph.get_node(global_pos.x, global_pos.y, global_pos.z);
            if (neighbor != nullptr && neighbor->level == tree->depth - level) {
                // Make connections both ways
                connection_node.connections[0].add_connection(neighbor->id);
                neighbor->connections[1].add_connection(connection_node.id);
                break;
            }  // If there is no connection but the neighbor node goes to a lower level
            else if (tree->depth - level <= connection_node.level) {
                // Go one level deeper to find a connection on the edges
                // +X edge mask
                uint64_t edge_mask = 0x8888888888888888ULL;
                set_lower_level_neighbors(graph, connection_node, tree, neighbor_node_index, tree->depth - level, (node_scale >> 2), global_pos.x, global_pos.y, global_pos.z, edge_mask, 0, 1);
                break;
            }

            node = &tree->nodes[neighbor_node_index];
        }
    }

    {  // -Z
        tmt::Svt64Node* node = &tree->nodes[0];
        glm::uvec3 global_pos = glm::uvec3(0u);
        for (uint32_t level = 1u; level < tree->depth; ++level) {
            if (node_scale > global_z) break;

            // Get position of neighbor on this level
            const uint32_t x_index = (global_x >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t y_index = (global_y >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t z_index = ((global_z - node_scale) >> ((tree->depth - level) * 2u)) & 3u;

            // Get node index of this neighbor
            const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

            // If no node exists on this position
            if ((node->child_mask & (1ull << child_index)) == 0u) break;

            const uint32_t child_pos = (uint32_t)__popcnt64(node->child_mask & ((1ull << child_index) - 1u));
            const uint32_t neighbor_node_index = node->abs_ptr() + child_pos;

            const uint16_t level_scale = 1u << ((tree->depth - level) * 2u);

            global_pos += glm::uvec3(x_index * level_scale, y_index * level_scale, z_index * level_scale);

            // Check if connection node exists for this depth on the neighbors position
            DestructionNode* neighbor = graph.get_node(global_pos.x, global_pos.y, global_pos.z);
            if (neighbor != nullptr && neighbor->level == tree->depth - level) {
                // Make connections both ways
                connection_node.connections[4].add_connection(neighbor->id);
                neighbor->connections[5].add_connection(connection_node.id);
                break;
            }  // If there is no connection but the neighbor node is on a lower level
            else if (tree->depth - level <= connection_node.level) {
                // Go one level deeper to find a connection on the edge
                // +Z edge mask
                uint64_t edge_mask = 0xF000F000F000F000ULL;
                set_lower_level_neighbors(
                    graph, connection_node, tree, neighbor_node_index, tree->depth - level, (node_scale >> 2), x_index * level_scale, y_index * level_scale, z_index * level_scale, edge_mask,
                    4, 5
                );
                break;
            }

            node = &tree->nodes[neighbor_node_index];
        }
    }

    {  // -Y
        tmt::Svt64Node* node = &tree->nodes[0];
        glm::uvec3 global_pos = glm::uvec3(0u);
        for (uint32_t level = 1u; level < tree->depth; ++level) {
            if (node_scale > global_y) break;

            // Get position of neighbor on this level
            const uint32_t x_index = (global_x >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t y_index = ((global_y - node_scale) >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t z_index = (global_z >> ((tree->depth - level) * 2u)) & 3u;

            // Get node index of this neighbor
            const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

            // If no node exists on this position
            if ((node->child_mask & (1ull << child_index)) == 0u) break;

            const uint32_t child_pos = (uint32_t)__popcnt64(node->child_mask & ((1ull << child_index) - 1u));
            const uint32_t neighbor_node_index = node->abs_ptr() + child_pos;

            const uint16_t level_scale = 1u << ((tree->depth - level) * 2u);

            global_pos += glm::uvec3(x_index * level_scale, y_index * level_scale, z_index * level_scale);

            // Check if connection node exists for this depth on the neighbors position
            DestructionNode* neighbor = graph.get_node(global_pos.x, global_pos.y, global_pos.z);
            if (neighbor != nullptr && neighbor->level == tree->depth - level) {
                // Make connections both ways
                connection_node.connections[2].add_connection(neighbor->id);
                neighbor->connections[3].add_connection(connection_node.id);
                break;
            }  // If there is no connection but the neighbor node is on a lower level
            else if (tree->depth - level <= connection_node.level) {
                // Go one level deeper to find a connection on the edge
                // +Y edge mask
                uint64_t edge_mask = 0xFFFF000000000000ULL;
                set_lower_level_neighbors(
                    graph, connection_node, tree, neighbor_node_index, tree->depth - level, (node_scale >> 2), x_index * level_scale, y_index * level_scale, z_index * level_scale, edge_mask,
                    2, 3
                );
                break;
            }

            node = &tree->nodes[neighbor_node_index];
        }
    }

    {  // +X
        tmt::Svt64Node* node = &tree->nodes[0];
        glm::uvec3 global_pos = glm::uvec3(0u);
        // Skip the deepest part of the tree (voxels)
        for (uint32_t level = 1u; level < tree->depth; ++level) {
            // Check if global_x + node_scale is in range of the tree
            if (global_x + node_scale >= (1u << (tree->depth * 2u))) break;

            // Get position of neighbor on this level
            const uint32_t x_index = ((global_x + node_scale) >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t y_index = (global_y >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t z_index = (global_z >> ((tree->depth - level) * 2u)) & 3u;

            // Get node index of this neighbor
            const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

            // If no node exists on this position and we are at a higher level
            if ((node->child_mask & (1ull << child_index)) == 0u) break;

            const uint32_t child_pos = (uint32_t)__popcnt64(node->child_mask & ((1ull << child_index) - 1u));
            const uint32_t neighbor_node_index = node->abs_ptr() + child_pos;

            const uint16_t level_scale = 1u << ((tree->depth - level) * 2u);

            global_pos += glm::uvec3(x_index * level_scale, y_index * level_scale, z_index * level_scale);

            // Check if connection node exists for this depth on the neighbors position
            DestructionNode* neighbor = graph.get_node(global_pos.x, global_pos.y, global_pos.z);
            if (neighbor != nullptr && neighbor->level == tree->depth - level) {
                // Make connections both ways
                connection_node.connections[1].add_connection(neighbor->id);
                neighbor->connections[0].add_connection(connection_node.id);
                break;
            }

            node = &tree->nodes[neighbor_node_index];
        }
    }

    {  // +Z
        tmt::Svt64Node* node = &tree->nodes[0];
        glm::uvec3 global_pos = glm::uvec3(0u);
        // Skip the deepest part of the tree (voxels)
        for (uint32_t level = 1u; level < tree->depth; ++level) {
            if (global_z + node_scale >= (1u << (tree->depth * 2u))) break;

            // Get position of neighbor on this level
            const uint32_t x_index = (global_x >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t y_index = (global_y >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t z_index = ((global_z + node_scale) >> ((tree->depth - level) * 2u)) & 3u;

            // Get node index of this neighbor
            const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

            // If no node exists on this position and we are at a higher level
            if ((node->child_mask & (1ull << child_index)) == 0u) break;

            const uint32_t child_pos = (uint32_t)__popcnt64(node->child_mask & ((1ull << child_index) - 1u));
            const uint32_t neighbor_node_index = node->abs_ptr() + child_pos;

            const uint16_t level_scale = 1u << ((tree->depth - level) * 2u);

            global_pos += glm::uvec3(x_index * level_scale, y_index * level_scale, z_index * level_scale);

            // Check if connection node exists for this depth on the neighbors position
            DestructionNode* neighbor = graph.get_node(global_pos.x, global_pos.y, global_pos.z);
            if (neighbor != nullptr && neighbor->level == tree->depth - level) {
                // Make connections both ways
                connection_node.connections[5].add_connection(neighbor->id);
                neighbor->connections[4].add_connection(connection_node.id);
                break;
            }

            node = &tree->nodes[neighbor_node_index];
        }
    }

    {  // +Y
        tmt::Svt64Node* node = &tree->nodes[0];
        glm::uvec3 global_pos = glm::uvec3(0u);
        // Skip the deepest part of the tree (voxels)
        for (uint32_t level = 1u; level < tree->depth; ++level) {
            if (global_y + node_scale >= (1u << (tree->depth * 2u))) break;

            // Get position of neighbor on this level
            const uint32_t x_index = (global_x >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t y_index = ((global_y + node_scale) >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t z_index = (global_z >> ((tree->depth - level) * 2u)) & 3u;

            // Get node index of this neighbor
            const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

            // If no node exists on this position and we are at a higher level
            if ((node->child_mask & (1ull << child_index)) == 0u) break;

            const uint32_t child_pos = (uint32_t)__popcnt64(node->child_mask & ((1ull << child_index) - 1u));
            const uint32_t neighbor_node_index = node->abs_ptr() + child_pos;

            const uint16_t level_scale = 1u << ((tree->depth - level) * 2u);

            global_pos += glm::uvec3(x_index * level_scale, y_index * level_scale, z_index * level_scale);

            // Check if connection node exists for this depth on the neighbors position
            DestructionNode* neighbor = graph.get_node(global_pos.x, global_pos.y, global_pos.z);
            if (neighbor != nullptr && neighbor->level == tree->depth - level) {
                // Make connections both ways
                connection_node.connections[3].add_connection(neighbor->id);
                neighbor->connections[2].add_connection(connection_node.id);
                break;
            }

            node = &tree->nodes[neighbor_node_index];
        }
    }
}

void recurse_generate_connection(
    Destructible& graph, Svt64* tree, const uint32_t node_index, const uint32_t current_depth, const uint32_t node_scale, const uint32_t global_x, const uint32_t global_y,
    const uint32_t global_z
) {
    // Get current node
    const tmt::Svt64Node& node = tree->nodes[node_index];

    // Calculate new scale for child nodes
    const uint32_t child_scale = node_scale >> 2u;

    // Copy mask for iteration
    uint64_t mask = node.child_mask;

    const uint8_t level = (uint8_t)tree->depth - (uint8_t)current_depth;

    // Alwasy skip root node, check if all children are solid (always make a node for leaf nodes)
    if ((current_depth != 0 && all_children_solid(tree, node_index)) || (level == 1 && mask != 0)) {
        // Create node in connection graph for this node
        DestructionNode connection_node;
        connection_node.level = level;
        connection_node.tree_node = node_index;
        connection_node.global_position = glm::uvec3(global_x, global_y, global_z);
        connection_node.id = graph.node_count++;
        graph.nodes.push_back(connection_node);
        const uint32_t node_id = Destruction::pos_to_node_id(global_x, global_y, global_z);
        graph.nodes_map[node_id] = connection_node.id;

        set_highest_level_neighbors(graph, graph.nodes.back(), tree, node_scale, global_x, global_y, global_z);
        return;
    }

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

        // Recursively go deeper until child is a leaf
        if (current_depth + 1 < tree->depth) {
            recurse_generate_connection(
                graph, tree, child_node_index, current_depth + 1, child_scale, global_x + local_x * child_scale, global_y + local_y * child_scale, global_z + local_z * child_scale
            );
        }

        // Clear bit
        mask &= ~(1ull << i);
    }
}

void recurse_regenerate_connection(
    Destructible& graph, Svt64* tree, const uint32_t node_index, const uint32_t current_depth, const uint32_t node_scale, const uint32_t global_x, const uint32_t global_y,
    const uint32_t global_z, glm::uvec3 min, glm::uvec3 max
) {
    // Get current node
    const tmt::Svt64Node& node = tree->nodes[node_index];

    const glm::uvec3 node_min = glm::uvec3(global_x, global_y, global_z);
    const glm::uvec3 node_max = node_min + glm::uvec3(node_scale);

    // Check if this node has overlap with the changed area
    // Return if there is no overlap
    if (node_max.x <= min.x || node_min.x >= max.x || node_max.y <= min.y || node_min.y >= max.y || node_max.z <= min.z || node_min.z >= max.z) {
        return;
    }

    // Check if this node has a connection node
    DestructionNode* connection_node = graph.get_node(global_x, global_y, global_z);
    if (connection_node != nullptr && connection_node->level == tree->depth - current_depth) {
        // Remove all connections from this node to its neighbors
        for (size_t i = 0; i < 6; i++) {
            for (size_t j = 0; j < connection_node->connections[i].node_indices.size(); j++) {
                uint32_t connection_node_index = connection_node->connections[i].node_indices[j];
                // Get connection node
                DestructionNode& neighbor_node = graph.nodes[connection_node_index];
                // Remove connection to this node from the neighbor
                for (size_t k = 0; k < neighbor_node.connections[i ^ 1].node_indices.size(); k++) {
                    if (neighbor_node.connections[i ^ 1].node_indices[k] == connection_node->id) {
                        neighbor_node.connections[i ^ 1].node_indices.erase(neighbor_node.connections[i ^ 1].node_indices.begin() + k);
                        break;
                    }
                }
            }
        }

        // TEMP
        connection_node->level = 10;

        // Remove node from hashmap
        graph.nodes_map.erase(Destruction::pos_to_node_id(connection_node->global_position.x, connection_node->global_position.y, connection_node->global_position.z));

        // Create new nodes recursively
        recurse_generate_connection(graph, tree, node_index, current_depth, node_scale, node_min.x, node_min.y, node_min.z);
    }

    // Calculate new scale for child nodes
    const uint32_t child_scale = node_scale >> 2u;

    // Copy mask for iteration
    uint64_t mask = node.child_mask;

    // NOTE: Because we are looping over the already destroyed tree, we never check the nodes already destroyed.

    // Recurse deeper into children
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

        // Recursively go deeper
        if (current_depth < tree->depth) {
            recurse_regenerate_connection(
                graph, tree, child_node_index, current_depth + 1, child_scale, global_x + local_x * child_scale, global_y + local_y * child_scale, global_z + local_z * child_scale, min, max
            );
        }

        // Clear bit
        mask &= ~(1ull << i);
    }
}

void Destruction::regenerate_connection_graph(Destructible& graph, Svt64* tree, const Stencil* stencil, glm::ivec3 offset) {
    TMT_ZONE_SCOPED

    // recurse_regenerate_connection(graph, tree, 0, 0, (1u << (tree->depth * 2u)), 0, 0, 0, (glm::uvec3)offset, (glm::uvec3)offset + stencil->size);
    //
    glm::uvec3 min = (glm::uvec3)offset;
    glm::uvec3 max = (glm::uvec3)offset + stencil->size;
    uint16_t node_count = (uint16_t)graph.nodes.size();
    // Loop over all nodes
    for (size_t n = 0; n < node_count; n++) {
        DestructionNode& node = graph.nodes[n];

        if (node.level == 10) continue;

        // AABB on all nodes
        uint32_t node_scale = (1u << (node.level * 2));
        glm::uvec3 node_min = node.global_position;
        glm::uvec3 node_max = node.global_position + glm::uvec3(node_scale);

        // AABB Check
        if (node_max.x <= min.x || node_min.x >= max.x || node_max.y <= min.y || node_min.y >= max.y || node_max.z <= min.z || node_min.z >= max.z) continue;

        // Remove all connections from this node to its neighbors
        for (size_t i = 0; i < 6; i++) {
            for (size_t j = 0; j < node.connections[i].node_indices.size(); j++) {
                uint32_t connection_node_index = node.connections[i].node_indices[j];
                // Get connection node
                DestructionNode& neighbor_node = graph.nodes[connection_node_index];
                // Remove connection to this node from the neighbor
                for (size_t k = 0; k < neighbor_node.connections[i ^ 1].node_indices.size(); k++) {
                    if (neighbor_node.connections[i ^ 1].node_indices[k] == node.id) {
                        neighbor_node.connections[i ^ 1].node_indices.erase(neighbor_node.connections[i ^ 1].node_indices.begin() + k);
                        break;
                    }
                }
            }
        }

        // Remove node from hashmap
        graph.nodes_map.erase(Destruction::pos_to_node_id(node.global_position.x, node.global_position.y, node.global_position.z));

        // Find tree node id from global position
        uint32_t node_index = 0;
        for (uint32_t level = 1u; level <= tree->depth - node.level; ++level) {
            tmt::Svt64Node* tree_node = &tree->nodes[node_index];
            // Get position of neighbor on this level
            const uint32_t x_index = (node.global_position.x >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t y_index = (node.global_position.y >> ((tree->depth - level) * 2u)) & 3u;
            const uint32_t z_index = (node.global_position.z >> ((tree->depth - level) * 2u)) & 3u;

            // Get node index of this neighbor
            const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

            // If no node exists on this position
            if ((tree_node->child_mask & (1ull << child_index)) == 0u) {
                node_index = 0;
                break;
            }

            const uint32_t child_pos = (uint32_t)__popcnt64(tree_node->child_mask & ((1ull << child_index) - 1u));
            const uint32_t child_node_index = tree_node->abs_ptr() + child_pos;

            node_index = child_node_index;
        }

        // TEMP
        uint8_t lvl = node.level;
        node.level = 10;

        // Don't generate new connections when node doesn't exist in the tree anymore
        if (node_index == 0) continue;

        // Create new nodes recursively (when we are not already on the lowest level)
        recurse_generate_connection(graph, tree, node_index, tree->depth - lvl, node_scale, node_min.x, node_min.y, node_min.z);
    }
}

void Destruction::generate_connection_graph(Destructible& graph, Svt64* tree) {
    TMT_ZONE_SCOPED
    // Scale of the root node in voxels
    const uint32_t node_scale = (1u << (tree->depth * 2u));
    recurse_generate_connection(graph, tree, 0, 0, node_scale, 0, 0, 0);
}

}  // namespace tmt