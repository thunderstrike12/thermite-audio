#include "svt64.hpp"

namespace tmt {

/* Log with base. */
inline uint32_t log_base(const uint32_t x, const uint32_t b) { return (uint32_t)ceil(log((double)x) / log((double)b)); }

/* Calculate the depth of a SVT64 based on its input voxel grid size. */
inline uint32_t tree_depth(uint32_t width, uint32_t height, uint32_t depth) {
    const float max_axis = (float)std::max(std::max(width, height), depth);
    return (uint32_t)(logf(max_axis) / logf(4.0f));
}

/* Calculate the maximum number of nodes a SVT64 can have given its depth. */
inline uint32_t max_node_count(uint32_t depth) {
    uint32_t node_count = 0u;
    for (int i = (int)depth - 1; i >= 0; --i) {
        const uint32_t width = (uint32_t)powf(4.0f, (float)i);
        node_count += width * width * width;
    }
    return node_count;
}

/* Find out how many solid voxels are inside of some raw voxel data. */
inline uint32_t raw_voxel_count(const RawVoxels& data) {
    uint32_t count = 0u;
    for (uint32_t z = 0u; z < data.d; ++z) {
        for (uint32_t y = 0u; y < data.h; ++y) {
            for (uint32_t x = 0u; x < data.w; ++x) {
                if (data.voxels[z * data.w * data.h + y * data.w + x] != 0u) count++;
            }
        }
    }
    return count;
}

Svt64Node::Svt64Node(const bool is_leaf, const uint32_t ptr, const uint64_t mask) {
    /* Only set the 31 least significant bits. */
    child_ptr = ptr & 0x7FFFFFFFu;
    child_mask = mask;

    /* Most significant bit is used to indicate a leaf node. */
    if (is_leaf) child_ptr |= 0x80000000u;
}

/* Recursive tree subdivide function. */
Svt64Node Svt64::subdivide(const RawVoxels& raw_data, int scale, glm::ivec3 index) {
    /* Create a leaf node */
    if (scale == 2) {
        Svt64Node leaf_node = Svt64Node(true, voxel_count, 0x00);

        /* Check if the node is outside the voxel grid bounds */
        if (index.x + 3u >= raw_data.w || index.y + 3u >= raw_data.h || index.z + 3u >= raw_data.d) return leaf_node;
        if (index.x < 0 || index.y < 0 || index.z < 0) return leaf_node;

        const int wh = raw_data.h * raw_data.w;

        for (int i = 0; i < 64; ++i) {
            /* Fetch the voxel data */
            const int voxel_x = index.x + ((i >> 0) & 3);
            const int voxel_y = index.y + ((i >> 4) & 3);
            const int voxel_z = index.z + ((i >> 2) & 3);
            const int voxel_offset = voxel_z * wh + voxel_y * raw_data.w + voxel_x;
            const MaterialIndex material = raw_data.voxels[voxel_offset];

            if (material > 0u) {
                voxels[voxel_count++] = material;
                leaf_node.child_mask |= (1ull << i);
            }
        }

        return leaf_node;
    }

    /* Descend */
    scale -= 2;

    /* Collect child nodes */
    Svt64Node child_nodes[64] {};
    uint64_t child_mask = 0x00u;
    uint32_t child_count = 0u;

    for (int i = 0; i < 64; ++i) {
        /* Subdivide the child node */
        const glm::ivec3 child_index = glm::ivec3(i >> 0 & 3, i >> 4 & 3, i >> 2 & 3);
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

void Svt64::build(const RawVoxels& raw_data) {
    /* Delete old data */
    if (nodes != nullptr) delete[] nodes;
    if (voxels != nullptr) delete[] voxels;

    /* Calculate the parameters of the 64 tree */
    depth = tree_depth(raw_data.w, raw_data.h, raw_data.d);
    const uint32_t max_nodes = max_node_count(depth);
    const uint32_t raw_voxels = raw_voxel_count(raw_data);

    /* Allocate space for new tree */
    nodes = new Svt64Node[max_nodes];  // (Node*)malloc(1ull << 26);
    node_count = 1u;
    voxels = new MaterialIndex[raw_voxels];  // (MaterialIndex*)malloc(1ull << 26);
    voxel_count = 0u;

    /* Copy the material palette */
    palette = raw_data.palette;

    /* Begin the recursive build */
    nodes[0] = subdivide(raw_data, depth * 2u, glm::ivec3(0));

    /* Reallocate the nodes to save memory */
    // nodes = (Svt64Node*)realloc(nodes, node_count * sizeof(Svt64Node) + SVT64_BUFFER_MEMORY);
}

Svt64::~Svt64() {
    if (nodes != nullptr) delete[] nodes;
    if (voxels != nullptr) delete[] voxels;
}

}  // namespace tmt
