#pragma once

#include "aabb.hpp"
#include "ray.hpp"

namespace tmt {

/* 2-wide Bounding Volume Hierarchy Node. */
struct Bvh2Node {
    /* Minimum bounds */
    glm::vec3 min_bounds {};
    /* Left child node, or first primitive index */
    uint32_t left_first = 0u;
    /* Maximum bounds */
    glm::vec3 max_bounds {};
    /* Primitive count */
    uint32_t prim_count = 0u;

    /* Returns true if this node is a leaf node. */
    inline bool is_leaf() const { return prim_count > 0u; }
};

/* 2-wide Bounding Volume Hierarchy Node as described by 2009 Aila & Laine. */
struct AilaLaineNode {
    glm::vec3 lmin {};
    uint32_t left = 0u;
    glm::vec3 lmax {};
    uint32_t right = 0u;
    glm::vec3 rmin {};
    uint32_t prim_index = 0u;
    glm::vec3 rmax {};
    uint32_t prim_count = 0u;
};

/*/
 * NOTE:
 * All BVH primitive types must implement the following functions:
 * 1. Aabb aabb() const;
 * 2. float intersect(const Ray&) const;
 * These are required for the BVH to function.
/*/

/* 2-wide Bounding Volume Hierarchy. */
template <typename T>
class Bvh2 {
   public:
    /* Primitive data */
    T* prims = nullptr;
    Aabb* bounds = nullptr;
    uint32_t prim_count = 0u;

    /* Nodes & primitive indices */
    Bvh2Node* nodes = nullptr;
    uint32_t* indices = nullptr;
    uint32_t node_count = 0u;

    /* Nodes optimized for gpu traversal */
    AilaLaineNode* gpu_nodes = nullptr;

    Bvh2() = default;
    ~Bvh2();

    /* No copies allowed */
    Bvh2(Bvh2&) = delete;
    Bvh2& operator=(Bvh2&) = delete;

    /* Build the acceleration structure. */
    void build(const T* input_prims, const uint32_t input_count);

    /* Trace the acceleration structure. */
    Hit trace(const Ray& ray) const;
};

}  // namespace tmt
