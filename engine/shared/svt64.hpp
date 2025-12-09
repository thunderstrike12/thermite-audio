#pragma once

#include <cstdint>
#include <vector>

#include "core/renderer/material.hpp"

namespace tmt {

/* How much space should be reserved for real-time modifications. */
constexpr uint32_t SVT64_BUFFER_MEMORY = 10000u /* 10 kb */;

struct RawVoxels {
    MaterialPalette palette {};
    std::vector<MaterialIndex> voxels {};
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
    Svt64Node subdivide(const RawVoxels& raw_data, int scale, glm::ivec3 index);

   public:
    /* List of tree nodes. */
    Svt64Node* nodes = nullptr;
    uint32_t node_count = 0u;

    /* List of voxel data. */
    MaterialPalette palette {};
    MaterialIndex* voxels = nullptr;
    uint32_t voxel_count = 0u;
    uint32_t depth = 0u;

    Svt64() = default;
    ~Svt64();

    /* Build the Sparse Voxel Tree. */
    void build(const RawVoxels& raw_data);
};

}  // namespace tmt
