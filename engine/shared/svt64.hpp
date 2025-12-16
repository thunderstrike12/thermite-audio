#pragma once

#include <cstdint>
#include <vector>

#include "engine/core/renderer/material.hpp"
#include "engine/systems/physics/physics_voxel_data.hpp"

namespace tmt {

/* How much space should be reserved for real-time modifications. */
constexpr uint32_t SVT64_BUFFER_MEMORY = 10000u /* 10 kb */;

struct RawVoxels {
    MaterialPalette palette {};
    std::vector<MaterialIndex> materials {};
    std::vector<PhysicsVoxel> physics_data {};
    uint32_t w = 0u, h = 0u, d = 0u;
};

struct VoxelHit {
    float t = 1e30f;
    glm::vec3 normal = glm::vec3(0.0f);
    uint8_t material {};
    uint16_t steps = 0u;

    VoxelHit() = default;
    VoxelHit(float t, glm::vec3 normal, uint8_t mat, uint16_t steps) : t(t), normal(normal), material(mat), steps(steps) {};
};

#pragma pack(push, 1)
struct Svt64Node {
    /* The most significant bit indicates if this node is a leaf containing voxels. */
    /* The 31 least significant bits are an absolute offset into an array of child nodes / voxels. */
    uint32_t child_ptr = 0u;
    /* Indicates which child nodes / voxels are present using 1 bit per child. */
    uint64_t child_mask = 0u;

    Svt64Node() = default;
    Svt64Node(const bool is_leaf, const uint32_t ptr, const uint64_t mask);

    /* Is this node a leaf node? */
    inline bool is_leaf() const { return (child_ptr >> 31u) == 1u; };

    /* Absolute offset into an array of child nodes / voxels. */
    inline uint32_t abs_ptr() const { return child_ptr & 0x7FFFFFFFu; };
};
#pragma pack(pop)

/* 64-wide Sparse Voxel Tree. */
class Svt64 {
    /* Recursive tree subdivide function. */
    Svt64Node subdivide(const RawVoxels& raw_data, uint32_t scale, glm::uvec3 index);

   public:
    /* List of tree nodes. */
    Svt64Node* nodes = nullptr;
    uint32_t node_count = 0u;

    /* List of voxel data. */
    MaterialPalette palette {};
    MaterialIndex* materials = nullptr;
    PhysicsVoxel* physics_data = nullptr;
    uint32_t voxel_count = 0u;
    uint32_t depth = 0u;

    Svt64() = default;
    ~Svt64();

    /* No copies allowed */
    Svt64(const Svt64& src) {
        node_count = src.node_count;
        voxel_count = src.voxel_count;
        depth = src.depth;
        nodes = new Svt64Node[node_count + SVT64_BUFFER_MEMORY / sizeof(Svt64Node)];
        materials = new MaterialIndex[voxel_count + SVT64_BUFFER_MEMORY / sizeof(MaterialIndex)];
        physics_data = new PhysicsVoxel[voxel_count + SVT64_BUFFER_MEMORY / sizeof(PhysicsVoxel)];
        memcpy(nodes, src.nodes, node_count * sizeof(Svt64Node));
        memcpy(materials, src.materials, voxel_count * sizeof(MaterialIndex));
        memcpy(physics_data, src.physics_data, voxel_count * sizeof(PhysicsVoxel));
        palette = src.palette;
    }
    Svt64& operator=(const Svt64& src) {
        node_count = src.node_count;
        voxel_count = src.voxel_count;
        depth = src.depth;
        nodes = new Svt64Node[node_count + SVT64_BUFFER_MEMORY / sizeof(Svt64Node)];
        materials = new MaterialIndex[voxel_count + SVT64_BUFFER_MEMORY / sizeof(MaterialIndex)];
        physics_data = new PhysicsVoxel[voxel_count + SVT64_BUFFER_MEMORY / sizeof(PhysicsVoxel)];
        memcpy(nodes, src.nodes, node_count * sizeof(Svt64Node));
        memcpy(materials, src.materials, voxel_count * sizeof(MaterialIndex));
        memcpy(physics_data, src.physics_data, voxel_count * sizeof(PhysicsVoxel));
        palette = src.palette;
        return *this;
    }

    bool is_empty(const uint32_t x, const uint32_t y, const uint32_t z);

    Material* get_voxel(const uint32_t x, const uint32_t y, const uint32_t z);
    PhysicsVoxel* get_physics_voxel(const uint32_t x, const uint32_t y, const uint32_t z);

    /* Build the Sparse Voxel Tree. */
    void build(const RawVoxels& raw_data);
};

}  // namespace tmt
