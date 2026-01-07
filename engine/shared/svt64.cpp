#include "svt64.hpp"
#include <nmmintrin.h>
#include "engine/tools/profiler.hpp"

namespace tmt {

/* Log with base. */
inline uint32_t log_base(const uint32_t x, const uint32_t b) { return (uint32_t)ceil(log((double)x) / log((double)b)); }

/* Calculate the depth of a SVT64 based on its input voxel grid size. */
inline uint32_t tree_depth(uint32_t width, uint32_t height, uint32_t depth) {
    TMT_ZONE_SCOPED

    const float max_axis = (float)std::max(std::max(width, height), depth);
    return (uint32_t)ceilf(logf(max_axis) / logf(4.0f));
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

bool Svt64::is_empty(const uint32_t x, const uint32_t y, const uint32_t z) {
    TMT_ZONE_SCOPED

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

        const uint32_t child_pos = __popcnt64(current->child_mask & ((1ull << child_index) - 1u));
        current = &nodes[current->abs_ptr() + child_pos];
    }

    return true;
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
            const uint32_t child_pos = __popcnt64(current->child_mask & ((1ull << child_index) - 1u));
            return &palette.entries[materials[current->abs_ptr() + child_pos]];
        }

        const uint32_t child_pos = __popcnt64(current->child_mask & ((1ull << child_index) - 1u));
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
            const uint32_t child_pos = __popcnt64(current->child_mask & ((1ull << child_index) - 1u));
            return &physics_data[current->abs_ptr() + child_pos];
        }

        const uint32_t child_pos = __popcnt64(current->child_mask & ((1ull << child_index) - 1u));
        current = &nodes[current->abs_ptr() + child_pos];
    }

    return nullptr;
}

void Svt64::build(const RawVoxels& raw_data) {
    TMT_ZONE_SCOPED

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
    nodes = new Svt64Node[max_nodes];  // (Node*)malloc(1ull << 26);
    node_count = 1u;
    materials = new MaterialIndex[raw_voxels + SVT64_BUFFER_MEMORY];  // (MaterialIndex*)malloc(1ull << 26);
    physics_data = new PhysicsVoxel[raw_voxels + SVT64_BUFFER_MEMORY];
    voxel_count = 0u;

    /* Copy the material palette */
    palette = raw_data.palette;

    /* Begin the recursive build */
    nodes[0] = subdivide(raw_data, depth * 2u, glm::uvec3(0u));

    /* Reallocate the nodes to save memory */
    nodes = (Svt64Node*)realloc(nodes, node_count * sizeof(Svt64Node) + SVT64_BUFFER_MEMORY);
}

Svt64::~Svt64() {
    if (depth > 0u) {
        delete[] nodes;
        delete[] materials;
        delete[] physics_data;
        depth = 0u;
    }
}

}  // namespace tmt
