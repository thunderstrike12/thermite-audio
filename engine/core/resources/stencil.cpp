#include "stencil.hpp"

#include <bit>
#include <cassert>

namespace tmt {

/* Recursively find the first model inside a voxel model hierarchy. */
const VoxelSceneNode* first_model(const VoxelSceneNode& parent);

bool Stencil::load() {
    // TEMP: It's currently just grabbing the first model node
    const VoxelSceneNode* model = nullptr;
    for (VoxelSceneNode& root_node : file_resource->root_nodes) {
        model = first_model(root_node);

        if (model != nullptr) break;  // If a first node was found in a root node, then we exit the loop.
    }

    if (model == nullptr || model->tree == nullptr) return false;

    // Initialize uniform grid sized to the model dimensions
    size = model->size;
    const uint32_t total = size.x * size.y * size.z;
    data.clear();
    data.resize(total, 0);

    // Traverse SVT64 and copy leaf solid voxels into the uniform grid (1 = solid, 0 = empty)
    Svt64* tree = model->tree.get();
    if (tree->depth == 0 || tree->nodes == nullptr) return true;  // nothing to copy

    const uint32_t root_extent = 1u << (tree->depth * 2u);

    copy_recursive(0u, glm::ivec3(0), root_extent, tree);

    // claudia made this:
    // recursive lambda to traverse nodes
    std::function<void(uint32_t, glm::ivec3, uint32_t)> traverse;
    traverse = [&](uint32_t node_id, glm::ivec3 node_pos, uint32_t node_scale) {
        const Svt64Node& node = tree->nodes[node_id];

        // If this node is a leaf, each bit in child_mask corresponds to a single voxel
        if (node.is_leaf()) {
            uint64_t mask = node.child_mask;
            while (mask != 0ull) {
                uint32_t voxel_id = std::countr_zero(mask);
                mask &= mask - 1;

                // decode packed index -> local coords
                const int lx = (voxel_id >> 0) & 3;
                const int ly = (voxel_id >> 4) & 3;
                const int lz = (voxel_id >> 2) & 3;

                glm::ivec3 world = node_pos + glm::ivec3(lx, ly, lz);

                // only write if inside model bounds
                if (world.x >= 0 && world.x < (int)size.x && world.y >= 0 && world.y < (int)size.y && world.z >= 0 && world.z < (int)size.z) {
                    const uint32_t idx = world.x + size.x * world.y + size.x * size.y * world.z;
                    data[idx] = 1;
                }
            }
            return;
        }

        // internal node: iterate present children and recurse
        uint64_t mask = node.child_mask;
        while (mask != 0ull) {
            uint32_t child_index = std::countr_zero(mask);
            mask &= mask - 1;

            // compute child pointer offset (popcount of bits before this child)
            uint32_t child_pos = std::popcount(node.child_mask & ((1ull << child_index) - 1ull));
            uint32_t child_node_id = node.abs_ptr() + child_pos;

            // decode local child coordinates
            const int cx = (child_index >> 0) & 3;
            const int cy = (child_index >> 4) & 3;
            const int cz = (child_index >> 2) & 3;

            uint32_t child_scale = node_scale >> 2;  // each level divides scale by 4
            glm::ivec3 child_pos_world = node_pos + glm::ivec3(cx, cy, cz) * (int)child_scale;

            // quick bounds check: if the child's AABB doesn't intersect the model extents, skip
            // child's max exclusive:
            glm::ivec3 child_max = child_pos_world + (int)child_scale;
            if (child_pos_world.x >= (int)size.x || child_pos_world.y >= (int)size.y || child_pos_world.z >= (int)size.z) continue;
            if (child_max.x <= 0 || child_max.y <= 0 || child_max.z <= 0) continue;

            traverse(child_node_id, child_pos_world, child_scale);
        }
    };
    return true;
}

void Stencil::unload() {
    data.clear();
    size = glm::uvec3(0u);
}

void Stencil::copy_recursive(const uint32_t node_id, const glm::ivec3 node_pos, const uint32_t node_scale, const Svt64* tree) {
    const Svt64Node& node = tree->nodes[node_id];

    // If this node is a leaf, each bit in child_mask corresponds to a single voxel
    if (node.is_leaf()) {
        // Loop over all solids
        uint64_t mask = node.child_mask;
        while (mask != 0ull) {
            // Get voxel index and clear bit
            uint32_t voxel_id = std::countr_zero(mask);
            mask &= mask - 1;

            // Get local coords from index
            const int lx = (voxel_id >> 0) & 3;
            const int ly = (voxel_id >> 4) & 3;
            const int lz = (voxel_id >> 2) & 3;

            glm::ivec3 world = node_pos + glm::ivec3(lx, ly, lz);

            // Only write if inside model bounds
            if (world.x >= 0 && world.x < (int)size.x && world.y >= 0 && world.y < (int)size.y && world.z >= 0 && world.z < (int)size.z) {
                const uint32_t idx = world.x + size.x * world.y + size.x * size.y * world.z;
                data[idx] = 1;
            }
        }
        return;
    }

    // Loop over all solid children
    uint64_t mask = node.child_mask;
    while (mask != 0ull) {
        // Get voxel index and clear bit
        uint32_t child_index = std::countr_zero(mask);
        mask &= mask - 1;

        // compute child pointer offset (popcount of bits before this child)
        uint32_t child_pos = std::popcount(node.child_mask & ((1ull << child_index) - 1ull));
        uint32_t child_node_id = node.abs_ptr() + child_pos;

        // Get local coords from index
        const int cx = (child_index >> 0) & 3;
        const int cy = (child_index >> 4) & 3;
        const int cz = (child_index >> 2) & 3;

        uint32_t child_scale = node_scale >> 2;  // each level divides scale by 4
        glm::ivec3 child_pos_world = node_pos + glm::ivec3(cx, cy, cz) * (int)child_scale;

        // quick bounds check: if the child's AABB doesn't intersect the model extents, skip
        // child's max exclusive:
        glm::ivec3 child_max = child_pos_world + (int)child_scale;
        if (child_pos_world.x >= (int)size.x || child_pos_world.y >= (int)size.y || child_pos_world.z >= (int)size.z) continue;
        if (child_max.x <= 0 || child_max.y <= 0 || child_max.z <= 0) continue;

        // Recurse into child
        copy_recursive(child_node_id, child_pos_world, child_scale, tree);
    }
}

}  // namespace tmt