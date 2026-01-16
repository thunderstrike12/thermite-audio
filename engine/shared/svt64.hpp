#pragma once

#include <vector>

#include "engine/core/renderer/material.hpp"
#include "engine/systems/physics/physics_voxel_data.hpp"

namespace tmt {

/* How much space should be reserved for real-time modifications. */
constexpr uint32_t SVT64_BUFFER_MEMORY = 1000u /* 1 kb */;

/* Raw uniform voxel input data. */
struct RawVoxels {
    MaterialPalette palette {};
    std::vector<MaterialIndex> materials {};
    std::vector<PhysicsVoxel> physics_data {};
    uint32_t w = 0u, h = 0u, d = 0u;
};

#pragma pack(push, 1)
/* 64-wide Sparse Voxel Tree Node. */
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

/* 64-wide Sparse Voxel Tree Ray Hit Data. */
struct Svt64Hit {
    glm::vec3 pos = glm::vec3(1e30f);
    uint32_t index = 0xFFFFFFFFu;
    glm::uvec3 coord {};

    Svt64Hit() = default;
    Svt64Hit(glm::vec3 p, uint32_t i, glm::uvec3 c) : pos(p), index(i), coord(c) {};
};

struct Ray;

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

    /* Copy constructor */
    Svt64(const Svt64& src);
    Svt64& operator=(const Svt64& src);

    /* @returns True if the given voxel coordinate is empty. */
    bool is_empty(const uint32_t x, const uint32_t y, const uint32_t z);

    /* @returns A pointer to the material of a voxel at the given coordinate. (nullptr if the voxel is emtpy) */
    Material* get_voxel(const uint32_t x, const uint32_t y, const uint32_t z);
    /* @returns A pointer to the physics data of a voxel at the given coordinate. (nullptr if the voxel is emtpy) */
    PhysicsVoxel* get_physics_voxel(const uint32_t x, const uint32_t y, const uint32_t z);

    /* Build the tree. */
    void build(const RawVoxels& raw_data);

    /* Trace a ray through the tree. */
    Svt64Hit trace(const Ray& ray) const;
};

}  // namespace tmt
