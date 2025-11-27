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

/* 2-wide Bounding Volume Hierarchy. */
class Bvh2 {
    /* Primitive data */
    Aabb* prims = nullptr;
    uint32_t prim_count = 0u;

    /* Nodes & primitive indices */
    Bvh2Node* nodes = nullptr;
    uint32_t* indices = nullptr;
    uint32_t node_count = 0u;

   public:
    Bvh2() = default;
    ~Bvh2();

    /* Build the acceleration structure. */
    void build(const Aabb* input_prims, const uint32_t input_count);

    /* Trace the acceleration structure. */
    Hit trace(const Ray& ray) const;
};

}  // namespace tmt
