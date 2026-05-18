#include "svt64.hpp"

#include <nmmintrin.h> /* popcnt64 */

#include "engine/tools/profiler.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/physics/components/destructable.hpp"

namespace tmt {

// Count number of set bits
inline uint32_t popcnt(uint64_t mask) {
    return (uint32_t)__popcnt64(mask);
}

// Count number of set bits in variable range [0..width]
inline uint32_t popcnt_var64(uint64_t mask, uint32_t width) {
    return (uint32_t)__popcnt64(mask & ((1ull << width) - 1));
}

/* Log with base. */
inline uint32_t log_base(const uint32_t x, const uint32_t b) {
    return (uint32_t)ceil(log((double)x) / log((double)b));
}

/* Calculate the depth of a SVT64 based on its input voxel grid size. */
inline uint32_t tree_depth(uint32_t width, uint32_t height, uint32_t depth) {
    TMT_ZONE_SCOPED

    const float max_axis = (float)glm::max(glm::max(width, height), depth);
    return (uint32_t)glm::max(1.0f, ceilf(logf(max_axis) / logf(4.0f)));
}

/* Calculate the maximum number of nodes a SVT64 can have given its depth. */
inline uint32_t max_node_count(uint32_t depth) {
    TMT_ZONE_SCOPED

    uint32_t node_count = 0u;
    for (int i = (int)depth - 1; i >= 0; --i) {
        const uint32_t width = (uint32_t)powf(4.0f, (float)i);
        node_count += width * width * width;
    }
    return node_count;
}

/* Find out how many solid voxels are inside of some raw voxel data. */
inline uint32_t raw_voxel_count(const RawVoxels& data) {
    TMT_ZONE_SCOPED

    uint32_t count = 0u;
    for (uint32_t z = 0u; z < data.d; ++z) {
        for (uint32_t y = 0u; y < data.h; ++y) {
            for (uint32_t x = 0u; x < data.w; ++x) {
                if (data.materials[z * data.w * data.h + y * data.w + x] != AIR_INDEX) count++;
            }
        }
    }
    return count;
}

Svt64Node::Svt64Node(const bool is_leaf, const uint32_t ptr, const uint64_t mask) {
    TMT_ZONE_SCOPED

    /* Only set the 31 least significant bits. */
    child_ptr = ptr & 0x7FFFFFFFu;
    child_mask = mask;

    /* Most significant bit is used to indicate a leaf node. */
    if (is_leaf) child_ptr |= 0x80000000u;
}

/* Recursive tree subdivide function. */
Svt64Node Svt64::subdivide(const RawVoxels& raw_data, uint32_t scale, glm::uvec3 index) {
    TMT_ZONE_SCOPED

    /* Create a leaf node */
    if (scale == 2u) {
        Svt64Node leaf_node = Svt64Node(true, voxel_count, 0x00);

        /* Check if the node is outside the voxel grid bounds */
        if (index.x >= raw_data.w || index.y >= raw_data.h || index.z >= raw_data.d) return leaf_node;

        const uint32_t wh = raw_data.h * raw_data.w;

        for (uint32_t i = 0u; i < 64u; ++i) {
            /* Fetch the voxel data */
            const uint32_t voxel_x = index.x + ((i >> 0u) & 3u);
            const uint32_t voxel_y = index.y + ((i >> 4u) & 3u);
            const uint32_t voxel_z = index.z + ((i >> 2u) & 3u);
            if (voxel_x >= raw_data.w || voxel_y >= raw_data.h || voxel_z >= raw_data.d) continue;
            const uint32_t voxel_offset = voxel_z * wh + voxel_y * raw_data.w + voxel_x;
            const MaterialIndex material = raw_data.materials[voxel_offset];

            if (material != AIR_INDEX) {
                materials[voxel_count] = material;
                physics_data[voxel_count++] = raw_data.physics_data[voxel_offset];
                leaf_node.child_mask |= (1ull << i);
            }
        }

        return leaf_node;
    }

    /* Descend */
    scale -= 2u;

    /* Collect child nodes */
    Svt64Node child_nodes[64] {};
    uint64_t child_mask = 0x00u;
    uint32_t child_count = 0u;

    for (uint32_t i = 0u; i < 64u; ++i) {
        /* Subdivide the child node */
        const glm::uvec3 child_index = glm::uvec3(i >> 0u & 3u, i >> 4u & 3u, i >> 2u & 3u);
        const Svt64Node child = subdivide(raw_data, scale, index + (child_index << scale));

        /* If the child is not empty */
        if (child.child_mask != 0u) {
            child_mask |= (1ull << i);
            child_nodes[child_count++] = child;
        }
    }

    /* Create a node */
    const Svt64Node node = Svt64Node(false, node_count, child_mask);

    /* Add child nodes into the nodes array */
    memcpy(nodes + node_count, child_nodes, child_count * sizeof(Svt64Node));
    node_count += child_count;
    return node;
}

/* Recursive tree subdivide function. */
Svt64Node Svt64::subdivide_masked(
    uint32_t scale, glm::uvec3 index, const Svt64* original_tree, uint32_t node_index, const std::vector<uint64_t>& tree_masks, uint8_t id, const Destructible& graph
) {
    TMT_ZONE_SCOPED

    /* Create a leaf node */
    if (scale == 2u) {
        Svt64Node leaf_node = Svt64Node(true, voxel_count, 0x00);

        // Copy voxels from the original tree
        uint64_t origin_mask = original_tree->nodes[node_index].child_mask;

        // Get destruction node, and check if it exists
        const DestructionNode* destruction_node = graph.get_node(index.x, index.y, index.z);
        if (destruction_node != nullptr && destruction_node->level == 1u && !destruction_node->cleared) {
            // Add the filled parts connected to the id to mask_accum
            uint64_t mask_accum = 0ull;
            for (const auto& filled : destruction_node->flood_masks) {
                if (filled.first != id) continue;
                mask_accum |= filled.second;
            }

            // Apply mask
            origin_mask &= mask_accum;
        }

        while (origin_mask != 0ull) {
            // Find index of first set bit
            const uint32_t i = std::countr_zero(origin_mask);

            /* Fetch the voxel data */
            const uint32_t child_node_offset = (uint32_t)__popcnt64(original_tree->nodes[node_index].child_mask & ((1ull << i) - 1u));
            const uint32_t child_node_index = original_tree->nodes[node_index].abs_ptr() + child_node_offset;

            const MaterialIndex material = original_tree->materials[child_node_index];

            // Only add it to the tree if its solid
            if (material != AIR_INDEX) {
                materials[voxel_count] = material;
                physics_data[voxel_count++] = original_tree->physics_data[child_node_index];
                leaf_node.child_mask |= (1ull << i);
            }

            // Remove looped over bit
            origin_mask &= ~(1ull << i);
        }

        return leaf_node;
    }

    /* Descend */
    scale -= 2u;

    /* Collect child nodes */
    Svt64Node child_nodes[64] {};
    uint64_t child_mask = 0x00u;
    uint32_t child_count = 0u;

    uint64_t origin_mask = original_tree->nodes[node_index].child_mask;
    while (origin_mask != 0ull) {
        // Find index of first set bit
        const uint32_t i = std::countr_zero(origin_mask);

        /* Subdivide the child node */
        const glm::uvec3 child_index = glm::uvec3(i >> 0u & 3u, i >> 4u & 3u, i >> 2u & 3u);

        const uint32_t child_node_offset = (uint32_t)__popcnt64(original_tree->nodes[node_index].child_mask & ((1ull << i) - 1u));
        const uint32_t child_node_index = original_tree->nodes[node_index].abs_ptr() + child_node_offset;

        // Check if the child its masked out before descending deeper
        if (tree_masks[child_node_index] != 0 && (tree_masks[child_node_index] & (1ull << id)) == 0ull) {
            origin_mask &= ~(1ull << i);
            continue;
        }

        const Svt64Node child = subdivide_masked(scale, index + (child_index << scale), original_tree, child_node_index, tree_masks, id, graph);

        /* If the child is not empty */
        if (child.child_mask != 0u) {
            child_mask |= (1ull << i);
            child_nodes[child_count++] = child;
        }

        // Remove looped over bit
        origin_mask &= ~(1ull << i);
    }

    /* Create a node */
    const Svt64Node node = Svt64Node(false, node_count, child_mask);

    /* Add child nodes into the nodes array */
    memcpy(nodes + node_count, child_nodes, child_count * sizeof(Svt64Node));
    node_count += child_count;
    return node;
}

constexpr uint64_t x_masks[4] = {
    0x8888888888888888ULL,  // x=3
    0x4444444444444444ULL,  // x=2
    0x2222222222222222ULL,  // x=1
    0x1111111111111111ULL,  // x=0
};

constexpr uint64_t z_masks[4] = {
    0xF000F000F000F000ULL,  // z=3
    0x0F000F000F000F00ULL,  // z=2
    0x00F000F000F000F0ULL,  // z=1
    0x000F000F000F000FULL,  // z=0
};

constexpr uint64_t y_masks[4] = {
    0xFFFF000000000000ULL,  // y=3
    0x0000FFFF00000000ULL,  // y=2
    0x00000000FFFF0000ULL,  // y=1
    0x000000000000FFFFULL,  // y=0
};

void get_max_x(Svt64* tree, uint32_t node_index, uint32_t depth, uint32_t x, uint32_t& max_x) {
    const tmt::Svt64Node& node = tree->nodes[node_index];

    for (size_t i = 0; i < 4; i++) {
        // Get mask and apply
        uint64_t current_mask = x_masks[i] & node.child_mask;
        if (current_mask == 0ull) continue;

        // Loop over all active nodes in this slice
        while (current_mask != 0ull) {
            // Get and clear bit
            const uint32_t bit = std::countr_zero(current_mask);
            current_mask &= ~(1ull << bit);

            // Get child x position
            const uint32_t child_scale = 1u << ((tree->depth - depth - 1) * 2u);
            const uint32_t child_x = x + ((bit >> 0u) & 3u) * child_scale;

            // Set maximum
            if (child_x > max_x) max_x = child_x;

            // Only recurse if not at leaf level
            if (depth + 1 < tree->depth) {
                const uint32_t child_offset = (uint32_t)__popcnt64(node.child_mask & ((1ull << bit) - 1u));
                const uint32_t child_node_index = node.abs_ptr() + child_offset;
                get_max_x(tree, child_node_index, depth + 1, child_x, max_x);
            }
        }

        // We don't need to check further because we loop from max to min
        return;
    }
}

void get_max_y(Svt64* tree, uint32_t node_index, uint32_t depth, uint32_t y, uint32_t& max_y) {
    const tmt::Svt64Node& node = tree->nodes[node_index];

    for (size_t i = 0; i < 4; i++) {
        // Get mask and apply
        uint64_t current_mask = y_masks[i] & node.child_mask;
        if (current_mask == 0ull) continue;

        // Loop over all active nodes in this slice
        while (current_mask != 0ull) {
            // Get and clear bit
            const uint32_t bit = std::countr_zero(current_mask);
            current_mask &= ~(1ull << bit);

            // Get child y position
            const uint32_t child_scale = 1u << ((tree->depth - depth - 1) * 2u);
            const uint32_t child_y = y + ((bit >> 4u) & 3u) * child_scale;

            // Set maximum
            if (child_y > max_y) max_y = child_y;

            // Only recurse if not at leaf level
            if (depth + 1 < tree->depth) {
                const uint32_t child_offset = (uint32_t)__popcnt64(node.child_mask & ((1ull << bit) - 1u));
                const uint32_t child_node_index = node.abs_ptr() + child_offset;
                get_max_y(tree, child_node_index, depth + 1, child_y, max_y);
            }
        }

        // We don't need to check further because we loop from max to min
        return;
    }
}

void get_max_z(Svt64* tree, uint32_t node_index, uint32_t depth, uint32_t z, uint32_t& max_z) {
    const tmt::Svt64Node& node = tree->nodes[node_index];

    for (size_t i = 0; i < 4; i++) {
        // Get mask and apply
        uint64_t current_mask = z_masks[i] & node.child_mask;
        if (current_mask == 0ull) continue;

        // Loop over all active nodes in this slice
        while (current_mask != 0ull) {
            // Get and clear bit
            const uint32_t bit = std::countr_zero(current_mask);
            current_mask &= ~(1ull << bit);

            // Get child z position
            const uint32_t child_scale = 1u << ((tree->depth - depth - 1) * 2u);
            const uint32_t child_z = z + ((bit >> 2u) & 3u) * child_scale;

            // Set maximum
            if (child_z > max_z) max_z = child_z;

            // Only recurse if not at leaf level
            if (depth + 1 < tree->depth) {
                const uint32_t child_offset = (uint32_t)__popcnt64(node.child_mask & ((1ull << bit) - 1u));
                const uint32_t child_node_index = node.abs_ptr() + child_offset;
                get_max_z(tree, child_node_index, depth + 1, child_z, max_z);
            }
        }

        // We don't need to check further because we loop from max to min
        return;
    }
}

glm::uvec3 Svt64::get_max() {
    glm::uvec3 max_pos(0);
    get_max_x(this, 0, 0, 0, max_pos.x);
    get_max_y(this, 0, 0, 0, max_pos.y);
    get_max_z(this, 0, 0, 0, max_pos.z);
    return max_pos;
}

bool Svt64::is_empty(const uint32_t x, const uint32_t y, const uint32_t z) {
    TMT_ZONE_SCOPED

    /* Bounds check */
    if (x >= (1u << (depth * 2u)) || y >= (1u << (depth * 2u)) || z >= (1u << (depth * 2u))) {
        return true;
    }

    Svt64Node* current = &nodes[0];

    for (uint32_t level = 1u; level <= depth; ++level) {
        const uint32_t x_index = (x >> ((depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((depth - level) * 2u)) & 3u;

        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);
        if ((current->child_mask & (1ull << child_index)) == 0u) {
            return true;
        } else if (level == depth) {
            return false;
        }

        const uint32_t child_pos = popcnt_var64(current->child_mask, child_index);
        current = &nodes[current->abs_ptr() + child_pos];
    }

    return true;
}

Svt64Node* Svt64::get_leaf(const uint32_t x, const uint32_t y, const uint32_t z) {
    Svt64Node* current = &nodes[0];

    for (uint32_t level = 1u; level < depth; ++level) {
        const uint32_t x_index = (x >> ((depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((depth - level) * 2u)) & 3u;

        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);
        if ((current->child_mask & (1ull << child_index)) == 0u) {  // No child node at this position, return nullptr
            return nullptr;
        } else if (level == depth - 1) {                            // Last level, return leaf node
            const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
            return &nodes[current->abs_ptr() + child_pos];
        }

        const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
        current = &nodes[current->abs_ptr() + child_pos];
    }

    return current;
}

Material* Svt64::get_voxel(const uint32_t x, const uint32_t y, const uint32_t z) {
    TMT_ZONE_SCOPED

    Svt64Node* current = &nodes[0];

    for (uint32_t level = 1u; level <= depth; ++level) {
        const uint32_t x_index = (x >> ((depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((depth - level) * 2u)) & 3u;

        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);
        if ((current->child_mask & (1ull << child_index)) == 0u) {
            return nullptr;
        } else if (level == depth) {
            const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
            return &palette.entries[materials[current->abs_ptr() + child_pos]];
        }

        const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
        current = &nodes[current->abs_ptr() + child_pos];
    }

    return nullptr;
}

PhysicsVoxel* Svt64::get_physics_voxel(const uint32_t x, const uint32_t y, const uint32_t z) {
    TMT_ZONE_SCOPED

    Svt64Node* current = &nodes[0];

    for (uint32_t level = 1u; level <= depth; ++level) {
        const uint32_t x_index = (x >> ((depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((depth - level) * 2u)) & 3u;

        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);
        if ((current->child_mask & (1ull << child_index)) == 0u) {
            return nullptr;
        } else if (level == depth) {
            const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
            return &physics_data[current->abs_ptr() + child_pos];
        }

        const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
        current = &nodes[current->abs_ptr() + child_pos];
    }

    return nullptr;
}

/* Remove child node from node without corrupting child indices. */
inline void evict_node(Svt64Node* node, Svt64Node* root_node, const uint32_t pos, const uint32_t idx) {
    /* Find the number of children present in the node */
    const uint32_t child_count = popcnt(node->child_mask);

    /* Remove the child at `pos` from the child mask */
    node->child_mask &= ~(1ull << pos);
    if (idx >= child_count) return; /* No re-alignment required */

    /* Shift child data over to keep indices aligned */
    const uint32_t ptr = node->abs_ptr() + idx;
    memmove(root_node + ptr, root_node + ptr + 1, (child_count - idx) * sizeof(Svt64Node));
}

/* Remove child node from node without corrupting child indices. */
inline void evict_voxel(Svt64* tree, Svt64Node* node, MaterialIndex* material_data, PhysicsVoxel* physics_data, const uint32_t pos, const uint32_t idx) {
    /* Find the number of children present in the node */
    const uint32_t child_count = popcnt(node->child_mask);

    /* Remove the child at `pos` from the child mask */
    node->child_mask &= ~(1ull << pos);
    if (idx >= child_count) return; /* No re-alignment required */

    /* Shift child data over to keep indices aligned */
    const uint32_t ptr = node->abs_ptr() + idx;
    memmove(material_data + ptr, material_data + ptr + 1, (child_count - idx) * sizeof(MaterialIndex));
    memmove(physics_data + ptr, physics_data + ptr + 1, (child_count - idx) * sizeof(PhysicsVoxel));
    tree->voxels_wasted++;
}

void Svt64::subtract(const Stencil* stencil, glm::ivec3 offset) {
    TMT_ZONE_SCOPED

    subtract_recursive(0, glm::ivec3(0), 1u << (depth * 2u), stencil, offset);
}

void Svt64::subtract_recursive(uint32_t node_id, glm::ivec3 node_pos, uint32_t node_scale, const Stencil* stencil, glm::ivec3 offset) {
    Svt64Node& node = nodes[node_id];
    uint32_t child_scale = node_scale >> 2;

    // If leaf node
    if (node.is_leaf()) {
        // memcpy all possible entries to avoid issues when shifting indices
        tmt::MaterialIndex palette_lookup[64];
        memcpy(palette_lookup, &materials[node.abs_ptr()], sizeof(tmt::MaterialIndex) * 64);

        // Loop over solid voxels
        uint64_t mask = node.child_mask;
        while (mask != 0ull) {
            // Get child index and clear bit
            const uint32_t voxel_pos = std::countr_zero(mask);
            mask &= mask - 1;

            // Calculate position of this child in the stencil
            const glm::ivec3 voxel_local = glm::ivec3((voxel_pos >> 0) & 3, (voxel_pos >> 4) & 3, (voxel_pos >> 2) & 3);
            const glm::ivec3 voxel_world = node_pos + voxel_local * (int)child_scale;
            const glm::ivec3 voxel_relative = voxel_world - offset;

            // Bounds check
            if (voxel_relative.x < 0 || voxel_relative.x >= (int)stencil->size.x) continue;
            if (voxel_relative.y < 0 || voxel_relative.y >= (int)stencil->size.y) continue;
            if (voxel_relative.z < 0 || voxel_relative.z >= (int)stencil->size.z) continue;

            // Check if stencil has solid voxel here
            const uint32_t stencil_index = voxel_relative.x + stencil->size.x * (voxel_relative.y + stencil->size.y * voxel_relative.z);
            if (stencil->data[stencil_index] == 0) continue;

            const uint32_t child_id = (uint32_t)__popcnt64(node.child_mask & ((1ull << voxel_pos) - 1u));
            // Remove voxel from this node
            evict_voxel(this, &node, materials, physics_data, voxel_pos, child_id);
        }

        return;
    }

    // Recurse into children if they overlap with the stencil
    uint64_t mask = node.child_mask;
    while (mask != 0ull) {
        // Get child index and clear bit
        const uint32_t child_id = std::countr_zero(mask);
        mask &= mask - 1;

        // Get child pointer
        const uint32_t child_ptr = std::popcount(node.child_mask & ((1ull << child_id) - 1));

        // Calculate child position
        const glm::ivec3 child_local = glm::ivec3((child_id >> 0) & 3, (child_id >> 4) & 3, (child_id >> 2) & 3);
        const glm::ivec3 child_pos = node_pos + child_local * (int)child_scale;

        // AABB overlap
        glm::ivec3 stencil_world_min = offset;
        glm::ivec3 stencil_world_max = stencil_world_min + glm::ivec3((int)stencil->size.x, (int)stencil->size.y, (int)stencil->size.z);
        glm::ivec3 child_max = child_pos + (int)child_scale;

        if (child_pos.x >= stencil_world_max.x || child_max.x <= stencil_world_min.x) continue;
        if (child_pos.y >= stencil_world_max.y || child_max.y <= stencil_world_min.y) continue;
        if (child_pos.z >= stencil_world_max.z || child_max.z <= stencil_world_min.z) continue;

        // Recurse into child
        subtract_recursive(node.abs_ptr() + child_ptr, child_pos, child_scale, stencil, offset);

        // Remove indices when all children become empty
        if (nodes[node.abs_ptr() + child_ptr].child_mask == 0) {
            evict_node(&node, nodes, child_id, child_ptr);
        }
    }
}

void Svt64::remove_voxel_dirty(const uint32_t x, const uint32_t y, const uint32_t z) {
    Svt64Node* current = &nodes[0];

    // Bounds check
    if (x >= (1u << (depth * 2u)) || y >= (1u << (depth * 2u)) || z >= (1u << (depth * 2u))) return;

    for (uint32_t level = 1u; level <= depth; ++level) {
        // Get child position
        const uint32_t x_index = (x >> ((depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((depth - level) * 2u)) & 3u;

        // Get child index
        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);

        // Check if child exists
        if ((current->child_mask & (1ull << child_index)) == 0u) {
            return;
        } else if (level == depth)  // Leaf node
        {
            const uint32_t num_voxels = (uint32_t)__popcnt64(current->child_mask);
            const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
            // Remove the voxel from the child mask
            current->child_mask &= ~(1ull << child_index);

            // Move all data relevant to this node over by 1
            memmove(&materials[current->abs_ptr() + child_pos], &materials[current->abs_ptr() + child_pos + 1u], (num_voxels - child_pos - 1u) * sizeof(MaterialIndex));

            return;
        }

        // Descend
        const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
        current = &nodes[current->abs_ptr() + child_pos];
    }
}

/* Convert absolute voxel coordinate to level local voxel coordinate. */
inline uint32_t get_level_local_pos(const uint32_t x, const uint32_t y, const uint32_t z, const uint32_t level) {
    const uint32_t level_2x = level << 1;             /* x2 */
    const uint32_t local_x = x >> level_2x & 0b11u;   /* % 4 */
    const uint32_t local_y = y >> level_2x & 0b11u;
    const uint32_t local_z = z >> level_2x & 0b11u;
    return local_x | (local_z << 2) | (local_y << 4); /* x & y need to be flipped */
}

void Svt64::remove_voxel(const uint32_t x, const uint32_t y, const uint32_t z) {
    /* Bounds check */
    const uint32_t tree_width = 1u << (depth * 2u);
    if (x >= tree_width || y >= tree_width || z >= tree_width) return;

    /* Traversal state */
    Svt64Node* node = &nodes[0];
    uint32_t level = depth - 1u;
    uint32_t stack[6] {};

    /* First, traverse down the tree, to find the voxel to remove */
    for (;;) {
        /* Get the sparse child node / voxel pointer */
        const uint32_t child_pos = get_level_local_pos(x, y, z, level);
        const uint32_t child_ptr = popcnt_var64(node->child_mask, child_pos);

        /* Check if the next child node / voxel is already empty */
        const bool is_emtpy = node->child_active(child_pos) == false;
        if (is_emtpy) return; /* Done */

        /* If we're at the bottom level we're done traversing down */
        if (level == 0u) {
            evict_voxel(this, node, materials, physics_data, child_pos, child_ptr);
            break;
        }

        /* Traverse down the tree, update the stack */
        stack[--level] = node->abs_ptr() + child_ptr;
        node = &nodes[stack[level]];
    }

    /* Next, traverse back up the tree, to update any masks that need updating */
    for (; node->child_mask == 0u && level < (depth - 1u);) {
        /* Fetch the next node up */
        node = &nodes[stack[++level]];

        /* Find the node position & pointer, remove the child */
        const uint32_t node_pos = get_level_local_pos(x, y, z, level);
        const uint32_t node_ptr = popcnt_var64(node->child_mask, node_pos);
        evict_node(node, nodes, node_pos, node_ptr);
    }
}

void Svt64::build(const RawVoxels& raw_data) {
    TMT_ZONE_SCOPED
    /* Bounds check */
    if (raw_data.w == 0u || raw_data.h == 0u || raw_data.d == 0u) return;
    if (raw_data.w > 1024u || raw_data.h > 1024u || raw_data.d > 1024u) return;

    /* Delete old data */
    if (depth > 0u) {
        delete[] nodes;
        delete[] materials;
        delete[] physics_data;
        depth = 0u;
    }

    /* Calculate the parameters of the 64 tree */
    depth = tree_depth(raw_data.w, raw_data.h, raw_data.d);
    const uint32_t max_nodes = max_node_count(depth);
    const uint32_t raw_voxels = raw_voxel_count(raw_data);

    /* Allocate space for new tree */
    nodes = new Svt64Node[max_nodes];
    node_count = 1u;
    materials = new MaterialIndex[raw_voxels + SVT64_BUFFER_MEMORY];
    physics_data = new PhysicsVoxel[raw_voxels + SVT64_BUFFER_MEMORY];
    voxels_capacity = raw_voxels + SVT64_BUFFER_MEMORY;
    voxel_count = 0u;

    /* Copy the material palette */
    palette = raw_data.palette;

    /* Begin the recursive build */
    nodes[0] = subdivide(raw_data, depth * 2u, glm::uvec3(0u));

    /* Reallocate the nodes to save memory */
    nodes = (Svt64Node*)realloc(nodes, (node_count + SVT64_BUFFER_MEMORY) * sizeof(Svt64Node));
    nodes_capacity = node_count + SVT64_BUFFER_MEMORY;
}

inline uint32_t get_node_cell_index(const glm::vec3 pos, const int scale_exp) {
    const uint32_t cell_x = (uint32_t&)pos.x >> scale_exp & 3u;
    const uint32_t cell_y = (uint32_t&)pos.y >> scale_exp & 3u;
    const uint32_t cell_z = (uint32_t&)pos.z >> scale_exp & 3u;
    return cell_x + cell_y * 16u + cell_z * 4u;
}

inline glm::vec3 floor_scale(const glm::vec3 pos, const int scale_exp) {
    const uint32_t mask = ~0u << scale_exp;
    const uint32_t masked_x = (uint32_t&)pos.x & mask;
    const uint32_t masked_y = (uint32_t&)pos.y & mask;
    const uint32_t masked_z = (uint32_t&)pos.z & mask;
    return glm::vec3((float&)masked_x, (float&)masked_y, (float&)masked_z);
}

// Reverses `pos` from range [1.0, 2.0) to (2.0, 1.0] if `dir > 0`.
inline glm::vec3 mirror_pos(const glm::vec3 pos, const glm::vec3 dir) {
    glm::vec3 mirrored {};
    mirrored.x = dir.x > 0.0f ? (3.0f - pos.x) : pos.x;
    mirrored.y = dir.y > 0.0f ? (3.0f - pos.y) : pos.y;
    mirrored.z = dir.z > 0.0f ? (3.0f - pos.z) : pos.z;
    return mirrored;
}

/* Get the index of the voxel at a given position and traversal scale. */
glm::uvec3 voxel_index(glm::vec3 pos, uint32_t scale_exp) {
    /* Create a bitmask of all the position bits */
    const uint32_t pos_mask = (1u << (23 - scale_exp)) - 1u;
    const uint32_t cell_x = (uint32_t&)pos.x >> scale_exp & pos_mask;
    const uint32_t cell_y = (uint32_t&)pos.y >> scale_exp & pos_mask;
    const uint32_t cell_z = (uint32_t&)pos.z >> scale_exp & pos_mask;
    return glm::uvec3(cell_x, cell_y, cell_z);
}

Svt64Hit Svt64::trace(const Ray& ray) const {
    /* Traversal state */
    uint32_t stack[11] {};
    int scale_exp = 21;       /* 23 mantissa bits - 2 */
    uint32_t node_index = 0u; /* root node */
    Svt64Node node = nodes[node_index];

    const glm::vec3 origin = mirror_pos(ray.origin, ray.dir);
    const glm::vec3 dir = ray.dir;

    /* Mirror coordinates to simplify cell intersections */
    uint32_t mirror_mask = 0x00;
    if (dir.x > 0.0f) mirror_mask |= 3u << 0;
    if (dir.y > 0.0f) mirror_mask |= 3u << 4;
    if (dir.z > 0.0f) mirror_mask |= 3u << 2;

    /* Safety clamp */
    glm::vec3 pos = clamp(origin, 1.0f, 1.9999999f);
    const glm::vec3 abs_dir = glm::max(glm::vec3(0.0001f), glm::abs(dir));
    glm::vec3 side_dist = glm::vec3(0.0f);
    int i = 0;

    for (i = 0; i < 256; i++) {
        uint32_t child_index = get_node_cell_index(pos, scale_exp) ^ mirror_mask;

        /* Descend down the tree until we find an empty node or leaf node */
        while ((node.child_mask >> child_index & 1) != 0 && !node.is_leaf()) {
            /* Push the current node on the stack (at `scale_exp / 2`) */
            stack[scale_exp >> 1] = node_index;

            /* Fetch the child node */
            node_index = node.abs_ptr() + popcnt_var64(node.child_mask, child_index);
            node = nodes[node_index];

            /* Decrease the scale & get the next child index */
            scale_exp -= 2;
            child_index = get_node_cell_index(pos, scale_exp) ^ mirror_mask;
        }

        /* If this node is a leaf, check if we hit a voxel */
        if (node.is_leaf() && (node.child_mask >> child_index & 1) != 0) break;

        /* Check if we can actually take a larger step based on the child mask */
        int sub_scale_exp = scale_exp;
        if ((node.child_mask >> (child_index & 0b101010) & 0x00330033) == 0) sub_scale_exp++;

        /* Compute next pos by intersecting with max cell sides */
        const glm::vec3 cell_min = floor_scale(pos, sub_scale_exp);

        side_dist = (cell_min - origin) / -abs_dir;
        float tmax = fminf(fminf(side_dist.x, side_dist.y), side_dist.z);

        const glm::ivec3 cell_min_i = glm::ivec3((int&)cell_min.x, (int&)cell_min.y, (int&)cell_min.z);

        glm::ivec3 neighbor_max = cell_min_i;
        neighbor_max.x += side_dist.x == tmax ? -1 : (1 << sub_scale_exp) - 1;
        neighbor_max.y += side_dist.y == tmax ? -1 : (1 << sub_scale_exp) - 1;
        neighbor_max.z += side_dist.z == tmax ? -1 : (1 << sub_scale_exp) - 1;

        /* Move to the entry point of our neighbour */
        pos = glm::min(origin - abs_dir * tmax, (glm::vec3&)neighbor_max);

        /* Find the first common ancestor node based on left-most carry bit */
        const glm::uvec3 diff_pos = glm::uvec3((uint32_t&)pos.x ^ (uint32_t&)cell_min.x, (uint32_t&)pos.y ^ (uint32_t&)cell_min.y, (uint32_t&)pos.z ^ (uint32_t&)cell_min.z);
        const int diff_exp = 31 - _lzcnt_u32((diff_pos.x | diff_pos.y | diff_pos.z) & 0xFFAAAAAA);

        /* Traverse back up the tree if we need to */
        if (diff_exp > scale_exp) {
            /* Break if we're exiting the root node */
            scale_exp = diff_exp;
            if (diff_exp > 21) break;

            /* Read the first common ancestor node from the stack */
            node_index = stack[scale_exp >> 1];
            node = nodes[node_index];
        }
    }

    /* If we ended in a leaf, we can gather the hit data we need */
    if (node.is_leaf() && scale_exp <= 21) {
        pos = mirror_pos(pos, dir);

        const float tmax = glm::min(glm::min(side_dist.x, side_dist.y), side_dist.z);
        const glm::bvec3 side_mask = glm::greaterThanEqual(glm::vec3(tmax), side_dist);
        const glm::vec3 normal = glm::vec3(side_mask.x ? -glm::sign(dir.x) : 0.0f, side_mask.y ? -glm::sign(dir.y) : 0.0f, side_mask.z ? -glm::sign(dir.z) : 0.0f);

        return Svt64Hit(pos, 0xFFFFFFFFu, normal, voxel_index(pos, scale_exp));
    }
    return Svt64Hit();
}

Svt64::~Svt64() {
    if (depth > 0u) {
        delete[] nodes;
        delete[] materials;
        delete[] physics_data;
        depth = 0u;
    }
}

/* Copy */
Svt64::Svt64(const Svt64& src) {
    node_count = src.node_count;
    voxel_count = src.voxel_count;
    nodes_wasted = src.nodes_wasted;
    voxels_wasted = src.voxels_wasted;
    nodes_capacity = src.nodes_capacity;
    voxels_capacity = src.voxels_capacity;
    depth = src.depth;
    delete[] nodes;
    nodes = new Svt64Node[node_count + SVT64_BUFFER_MEMORY];
    delete[] materials;
    materials = new MaterialIndex[voxel_count + SVT64_BUFFER_MEMORY];
    delete[] physics_data;
    physics_data = new PhysicsVoxel[voxel_count + SVT64_BUFFER_MEMORY];
    memcpy(nodes, src.nodes, node_count * sizeof(Svt64Node));
    memcpy(materials, src.materials, voxel_count * sizeof(MaterialIndex));
    memcpy(physics_data, src.physics_data, voxel_count * sizeof(PhysicsVoxel));
    palette = src.palette;
}

/* Copy */
Svt64& Svt64::operator=(const Svt64& src) {
    node_count = src.node_count;
    voxel_count = src.voxel_count;
    nodes_wasted = src.nodes_wasted;
    voxels_wasted = src.voxels_wasted;
    nodes_capacity = src.nodes_capacity;
    voxels_capacity = src.voxels_capacity;
    depth = src.depth;
    delete[] nodes;
    nodes = new Svt64Node[node_count + SVT64_BUFFER_MEMORY];
    delete[] materials;
    materials = new MaterialIndex[voxel_count + SVT64_BUFFER_MEMORY];
    delete[] physics_data;
    physics_data = new PhysicsVoxel[voxel_count + SVT64_BUFFER_MEMORY];
    memcpy(nodes, src.nodes, node_count * sizeof(Svt64Node));
    memcpy(materials, src.materials, voxel_count * sizeof(MaterialIndex));
    memcpy(physics_data, src.physics_data, voxel_count * sizeof(PhysicsVoxel));
    palette = src.palette;
    return *this;
}

void Svt64::set_voxel(const uint32_t x, const uint32_t y, const uint32_t z, const MaterialIndex material) {
    /* Check if the voxel is in bounds */
    const uint32_t width = (uint32_t)powf(4.0f, (float)depth);
    if (x >= width || y >= width || z >= width) {
        Log::warn("SVT64 tried to set voxel outside bounds.");
        return;
    }

    /* Check if we need to defragment the tree */
    if ((node_count + SVT64_DEFRAG_THRESHOLD) >= nodes_capacity || (voxel_count + SVT64_DEFRAG_THRESHOLD) >= voxels_capacity) {
        Log::info("Running SVT64 defragmentation.");
        defrag();
    }

    uint32_t node_index = 0u;
    uint32_t scale = depth * 2u - 2u;

    for (;;) {
        Svt64Node& node = nodes[node_index];

        if (node.is_leaf()) break;

        const uint32_t lx = (x >> scale) & 3u;
        const uint32_t ly = (y >> scale) & 3u;
        const uint32_t lz = (z >> scale) & 3u;
        const uint32_t li = lx + ly * 16u + lz * 4u;
        const uint64_t lm = 1ull << li;

        /* If the child doesn't exist yet, create it */
        if ((node.child_mask & lm) == 0ull) {
            const uint32_t prev_child_index = node.abs_ptr();
            const uint32_t prev_nodes_count = popcnt(node.child_mask);
            // if (node_count + prev_nodes_count + 64u >= nodes_capacity) return;
            nodes_wasted += prev_nodes_count;
            node = Svt64Node(false, node_count, node.child_mask | lm);

            for (uint32_t i = 0u, j = 0u; i < 64u; ++i) {
                const bool is_new = i == li;

                if (is_new) {
                    /* Add new node */
                    nodes[node_count++] = Svt64Node(scale <= 2u, 0u, 0ull);
                } else {
                    /* Copy over old node */
                    if (node.child_mask & (1ull << i)) {
                        nodes[node_count++] = nodes[prev_child_index + j];
                        j++;
                    }
                }
            }
        }

        scale -= 2u;
        node_index = node.abs_ptr() + popcnt_var64(node.child_mask, li);
    }

    Svt64Node& node = nodes[node_index];

    const uint32_t lx = x & 3u;
    const uint32_t ly = y & 3u;
    const uint32_t lz = z & 3u;
    const uint32_t li = lx + ly * 16u + lz * 4u;
    const uint64_t lm = 1ull << li;

    /* If the voxel doesn't exist yet, create it */
    if ((node.child_mask & lm) == 0ull) {
        const uint32_t prev_voxel_index = node.abs_ptr();
        const uint32_t prev_voxel_count = popcnt(node.child_mask);
        // if (voxel_count + prev_voxel_count + 64u >= voxels_capacity) return;
        voxels_wasted += prev_voxel_count;

        node = Svt64Node(true, voxel_count, node.child_mask | lm);
        for (uint32_t i = 0u, j = 0u; i < 64u; ++i) {
            const bool is_new = i == li;

            if (is_new) {
                /* Add new voxel */
                materials[voxel_count++] = material;
            } else {
                /* Copy over old voxel */
                if (node.child_mask & (1ull << i)) {
                    materials[voxel_count++] = materials[prev_voxel_index + j];
                    j++;
                }
            }
        }
    } else {
        const uint32_t voxel_index = node.abs_ptr() + popcnt_var64(node.child_mask, li);
        materials[voxel_index] = material;
    }
}

void Svt64::defrag() {
    /* Copy current nodes and voxels */
    Svt64Node* new_nodes = new Svt64Node[node_count + SVT64_BUFFER_MEMORY];
    MaterialIndex* new_materials = new MaterialIndex[voxel_count + SVT64_BUFFER_MEMORY];
    PhysicsVoxel* new_physics_data = new PhysicsVoxel[voxel_count + SVT64_BUFFER_MEMORY];

    /* Traverse the tree top down, move all child nodes/voxels into new lists */
    nodes_capacity = node_count + SVT64_BUFFER_MEMORY;
    voxels_capacity = voxel_count + SVT64_BUFFER_MEMORY;
    node_count = 1u;
    voxel_count = voxels_wasted = nodes_wasted = 0u;

    /* Non-recursive */
    std::vector<uint32_t> old_stack(64 * depth);
    std::vector<uint32_t> new_stack(64 * depth);
    uint32_t stack_ptr = 0u;
    uint32_t old_node_index = 0u;
    uint32_t new_node_index = 0u;

    for (;;) {
        const Svt64Node& old_node = nodes[old_node_index];
        Svt64Node& new_node = new_nodes[new_node_index];
        const uint32_t child_count = popcnt(old_node.child_mask);
        const uint32_t child_ptr = old_node.abs_ptr();

        /* Defrag leaf node */
        if (old_node.is_leaf()) {
            new_node = Svt64Node(true, voxel_count, old_node.child_mask);
            memcpy(new_materials + voxel_count, materials + child_ptr, child_count * sizeof(MaterialIndex));
            memcpy(new_physics_data + voxel_count, physics_data + child_ptr, child_count * sizeof(PhysicsVoxel));
            voxel_count += child_count;
            if (stack_ptr == 0u) break;
            old_node_index = old_stack[--stack_ptr];
            new_node_index = new_stack[stack_ptr];
            continue;
        }

        /* Defrag node */
        new_node = Svt64Node(false, node_count, old_node.child_mask);
        memcpy(new_nodes + node_count, nodes + child_ptr, child_count * sizeof(Svt64Node));
        node_count += child_count;

        /* Recurse into child nodes */
        for (uint32_t i = 0u; i < child_count; ++i) {
            old_stack[stack_ptr] = child_ptr + i;
            new_stack[stack_ptr++] = new_node.abs_ptr() + i;
        }

        if (stack_ptr == 0u) break;
        old_node_index = old_stack[--stack_ptr];
        new_node_index = new_stack[stack_ptr];
    }

    delete[] nodes;
    nodes = new_nodes;
    delete[] materials;
    materials = new_materials;
    delete[] physics_data;
    physics_data = new_physics_data;
}

}  // namespace tmt
