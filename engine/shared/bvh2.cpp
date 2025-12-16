#include "bvh2.hpp"

#include "engine/core/renderer/voxel_object.hpp"

namespace tmt {

/* Number of bins to use during BVH construction. */
constexpr int BVH_BINS = 8;
constexpr int BVH_SPLITS = BVH_BINS - 1;

/* Returns the surface area of a node. */
inline float surface_area(const Bvh2Node& node) {
    const glm::vec3 extent = node.max_bounds - node.min_bounds;
    return extent.x * extent.y + extent.y * extent.z + extent.z * extent.x;
}

/* Returns half the area of an extent `v`. */
inline float half_area(const glm::vec3& v) { return v.x < -BIG_F32 ? 0.0f : (v.x * v.y + v.y * v.z + v.z * v.x); }

template <typename T>
void Bvh2<T>::build(const T* input_prims, const uint32_t input_count) {
    /* Delete old BVH data if it exists */
    if (node_count > 0u) {
        delete[] nodes;
        delete[] prims;
        delete[] bounds;
        delete[] indices;
        delete[] gpu_nodes;
        node_count = 0u;
    }

    /* Allocate space for nodes */
    prim_count = input_count;
    const uint32_t nodes_required = prim_count * 2u + 1u;
    nodes = new Bvh2Node[nodes_required] {};
    node_count = 2u; /* Skip the 2nd node for better cache-line alignment */

    /* Copy the input primitives */
    prims = new T[prim_count] {};
    bounds = new Aabb[prim_count] {};
    memcpy(prims, input_prims, prim_count * sizeof(T));

    /* Initialize primitive indices */
    indices = new uint32_t[prim_count] {};
    for (uint32_t i = 0u; i < prim_count; ++i) indices[i] = i;

    /* Setup the root node for the BVH */
    Bvh2Node& root = nodes[0];
    root.left_first = 0u;
    root.prim_count = prim_count;
    root.min_bounds = glm::vec3(BIG_F32);
    root.max_bounds = glm::vec3(-BIG_F32);
    /* Generate all primitive AABBs, and find the root AABB */
    for (uint32_t i = 0u; i < prim_count; ++i) {
        bounds[i] = prims[i].aabb();
        root.min_bounds.x = fminf(root.min_bounds.x, bounds[i].min.x);
        root.min_bounds.y = fminf(root.min_bounds.y, bounds[i].min.y);
        root.min_bounds.z = fminf(root.min_bounds.z, bounds[i].min.z);
        root.max_bounds.x = fmaxf(root.max_bounds.x, bounds[i].max.x);
        root.max_bounds.y = fmaxf(root.max_bounds.y, bounds[i].max.y);
        root.max_bounds.z = fmaxf(root.max_bounds.z, bounds[i].max.z);
    }

    /* If there are no primitives, set root bounds to zero */
    if (prim_count == 0u) {
        root.min_bounds = glm::vec3(0.0f);
        root.max_bounds = glm::vec3(0.0f);
    }

    /* Build the node hierarchy */
    uint32_t build_tasks[256] {}, task_count = 0u, node_ptr = 0u;
    const glm::vec3 min_extent = (root.max_bounds - root.min_bounds) * 1e-20f;
    glm::vec3 best_lmin {}, best_lmax {}, best_rmin {}, best_rmax {};
    for (;;) {
        for (;;) {
            Bvh2Node& node = nodes[node_ptr];

            /* Initialize the primitive bins */
            glm::vec3 bin_min[3][BVH_BINS] {}, bin_max[3][BVH_BINS] {};
            for (uint32_t a = 0; a < 3u; ++a) {
                for (uint32_t i = 0; i < BVH_BINS; ++i) {
                    bin_min[a][i] = glm::vec3(BIG_F32);
                    bin_max[a][i] = glm::vec3(-BIG_F32);
                }
            }

            /* Scatter the primitives into the bins */
            uint32_t bin_count[3][BVH_BINS] {};
            const glm::vec3 rpd3 = glm::vec3((float)BVH_BINS / (node.max_bounds - node.min_bounds));
            const glm::vec3 nmin3 = node.min_bounds;
            for (uint32_t i = 0u; i < node.prim_count; ++i) {
                const Aabb& prim = bounds[indices[node.left_first + i]];
                glm::ivec3 bi = glm::ivec3(((prim.min + prim.max) * 0.5f - nmin3) * rpd3);
                bi.x = glm::clamp(bi.x, 0, BVH_BINS - 1);
                bi.y = glm::clamp(bi.y, 0, BVH_BINS - 1);
                bi.z = glm::clamp(bi.z, 0, BVH_BINS - 1);
                bin_min[0][bi.x] = glm::min(bin_min[0][bi.x], prim.min);
                bin_max[0][bi.x] = glm::max(bin_max[0][bi.x], prim.max), bin_count[0][bi.x]++;
                bin_min[1][bi.y] = glm::min(bin_min[1][bi.y], prim.min);
                bin_max[1][bi.y] = glm::max(bin_max[1][bi.y], prim.max), bin_count[1][bi.y]++;
                bin_min[2][bi.z] = glm::min(bin_min[2][bi.z], prim.min);
                bin_max[2][bi.z] = glm::max(bin_max[2][bi.z], prim.max), bin_count[2][bi.z]++;
            }

            /* Calculate the cost for each split */
            float split_cost = BIG_F32;
            uint32_t best_axis = 0u, best_split = 0u;
            for (uint32_t a = 0u; a < 3u; ++a) {
                /* Skip if the node is too small */
                if ((node.max_bounds[a] - node.min_bounds[a]) <= min_extent[a]) continue;

                /* Evaluate the cost of each bin configuration */
                glm::vec3 lb_min[BVH_SPLITS] {}, rb_min[BVH_SPLITS] {}, l1 {BIG_F32}, l2 {-BIG_F32};
                glm::vec3 lb_max[BVH_SPLITS] {}, rb_max[BVH_SPLITS] {}, r1 {BIG_F32}, r2 {-BIG_F32};
                float l_cost[BVH_SPLITS] {}, r_cost[BVH_SPLITS] {};
                for (uint32_t ln = 0u, rn = 0u, i = 0u; i < BVH_SPLITS; ++i) {
                    lb_min[i] = l1 = glm::min(l1, bin_min[a][i]);
                    rb_min[BVH_BINS - 2u - i] = r1 = glm::min(r1, bin_min[a][BVH_SPLITS - i]);
                    lb_max[i] = l2 = glm::max(l2, bin_max[a][i]);
                    rb_max[BVH_BINS - 2u - i] = r2 = glm::max(r2, bin_max[a][BVH_SPLITS - i]);
                    ln += bin_count[a][i], rn += bin_count[a][BVH_SPLITS - i];
                    l_cost[i] = ln == 0u ? BIG_F32 : (half_area(l2 - l1) * (float)ln);
                    r_cost[BVH_BINS - 2u - i] = rn == 0u ? BIG_F32 : (half_area(r2 - r1) * (float)rn);
                }

                /* Find the split with the lowest cost */
                for (uint32_t i = 0u; i < BVH_BINS - 1u; ++i) {
                    const float cost = l_cost[i] + r_cost[i];
                    if (cost >= split_cost) continue;

                    split_cost = cost, best_axis = a, best_split = i;
                    best_lmin = lb_min[i], best_rmin = rb_min[i], best_lmax = lb_max[i], best_rmax = rb_max[i];
                }
            }

            /* Calculate the actual split cost */
            split_cost = 1.0f + split_cost / surface_area(node);

            /* Break if the split is not worth it (if 3 or less primitives) */
            if (node.prim_count < 4u && split_cost >= node.prim_count) {
                break;
            }

            /* Sort the primitve indices */
            uint32_t j = node.left_first + node.prim_count, src = node.left_first;
            const float rpd = rpd3[best_axis], nmin = nmin3[best_axis];
            for (uint32_t i = 0u; i < node.prim_count; ++i) {
                const Aabb& prim = bounds[indices[src]];
                const uint32_t bi = glm::clamp((uint32_t)(((prim.min[best_axis] + prim.max[best_axis]) * 0.5f - nmin) * rpd), 0u, BVH_BINS - 1u);
                if (bi <= best_split) {
                    src++;
                } else {
                    std::swap(indices[src], indices[--j]);
                }
            }

            /* Create two child nodes */
            const uint32_t left_count = src - node.left_first, right_count = node.prim_count - left_count;
            if (left_count == 0u || right_count == 0u) break;
            const uint32_t n = node_count;
            node_count += 2u;
            nodes[n].min_bounds = best_lmin, nodes[n].max_bounds = best_lmax;
            nodes[n].left_first = node.left_first, nodes[n].prim_count = left_count;
            nodes[n + 1u].min_bounds = best_rmin, nodes[n + 1u].max_bounds = best_rmax;
            nodes[n + 1u].left_first = j, nodes[n + 1u].prim_count = right_count;
            node.left_first = n, node.prim_count = 0u;

            /* Add one of the two child nodes onto the build stack */
            build_tasks[task_count++] = n + 1u, node_ptr = n;
        }

        /* Fetch task from the build stack */
        if (task_count == 0u) break;
        node_ptr = build_tasks[--task_count];
    }

    /* Convert the BVH2 into the GPU friendly format */
    gpu_nodes = new AilaLaineNode[node_count] {};
    uint32_t alt_node = 0u, node_index = 0u, stack[128] {}, stack_ptr = 0u;
    for (;;) { /* Credit: <https://github.com/jbikker/tinybvh> */
        const Bvh2Node& node = nodes[node_index];
        const uint32_t idx = alt_node++;
        /* Handle case where the tree is only a root node */
        if ((node_index == 0u && node.left_first == 0u) || node.is_leaf()) {
            gpu_nodes[idx].prim_count = node.prim_count;
            gpu_nodes[idx].prim_index = node.left_first;
            if (!stack_ptr) break;
            node_index = stack[--stack_ptr];
            const uint32_t new_parent = stack[--stack_ptr];
            gpu_nodes[new_parent].right = alt_node; /* <- right filled here */
            continue;
        }
        const Bvh2Node& left = nodes[node.left_first];
        const Bvh2Node& right = nodes[node.left_first + 1];
        gpu_nodes[idx].lmin = left.min_bounds, gpu_nodes[idx].rmin = right.min_bounds;
        gpu_nodes[idx].lmax = left.max_bounds, gpu_nodes[idx].rmax = right.max_bounds;
        gpu_nodes[idx].left = alt_node; /* right will be filled when popped! */
        stack[stack_ptr++] = idx;
        stack[stack_ptr++] = node.left_first + 1;
        node_index = node.left_first;
    }
}

template <typename T>
std::vector<uint32_t> Bvh2<T>::overlap(const Aabb& aabb) const {
    /* Traversal state */
    std::vector<uint32_t> hits {};
    uint32_t stack[32] {}, stack_ptr = 0u, node_index = 0u;

    for (;;) {
        const Bvh2Node& node = nodes[node_index];

        /* Leaf node */
        if (node.is_leaf()) {
            /* Intersect all primitives */
            for (uint32_t i = 0u; i < node.prim_count; ++i) {
                const uint32_t index = indices[node.left_first + i];
                if (bounds[index].overlap(aabb) == false) continue;
                hits.push_back(index);
            }

            /* Pop the node stack */
            if (stack_ptr == 0u) break;
            node_index = stack[--stack_ptr];
            continue;
        }

        /* Interior node */
        uint32_t child1_index = node.left_first;
        uint32_t child2_index = node.left_first + 1u;
        const Bvh2Node &child1 = nodes[child1_index], &child2 = nodes[child2_index];
        bool hit1 = child1.aabb().overlap(aabb);
        bool hit2 = child2.aabb().overlap(aabb);

        /* Swap child nodes so that the closest one comes first */
        if ((hit2 == true) && (hit1 == false)) {
            std::swap(hit1, hit2), std::swap(child1_index, child2_index);
        }

        /* If we missed both child nodes */
        if (hit1 == false) {
            /* Pop the node stack */
            if (stack_ptr == 0u) break;
            node_index = stack[--stack_ptr];
        } else {
            /* Continue with the closest child node */
            node_index = child1_index;
            /* Push the 2nd child onto the node stack if we hit it */
            if (hit2 == true) stack[stack_ptr++] = child2_index;
        }
    }

    return hits;
}

/* Ray AABB intersection function. */
inline float intersect_aabb(const Ray ray, const glm::vec3 box_min, const glm::vec3 box_max) {
    const glm::vec3 t_to_min = (box_min - ray.origin) * ray.rcp_dir;
    const glm::vec3 t_to_max = (box_max - ray.origin) * ray.rcp_dir;
    const glm::vec3 t_min = glm::min(t_to_min, t_to_max);
    const glm::vec3 t_max = glm::max(t_to_min, t_to_max);
    const float t_near = glm::max(glm::max(glm::max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = glm::min(glm::min(t_max.x, t_max.y), t_max.z);
    return t_near > t_far ? BIG_F32 : t_near;
}

template <typename T>
Hit Bvh2<T>::trace(const Ray& ray) const {
    /* Traversal state */
    uint32_t stack[32] {}, stack_ptr = 0u, node_index = 0u;
    float min_t = BIG_F32;

    for (;;) {
        const Bvh2Node& node = nodes[node_index];

        /* Leaf node */
        if (node.is_leaf()) {
            /* Intersect all primitives */
            for (uint32_t i = 0u; i < node.prim_count; ++i) {
                const T& prim = prims[indices[node.left_first + i]];
                const float dist = prim.intersect(ray);
                min_t = fminf(min_t, dist);
            }

            /* Pop the node stack */
            if (stack_ptr == 0u) break;
            node_index = stack[--stack_ptr];
            continue;
        }

        /* Interior node */
        uint32_t child1_index = node.left_first;
        uint32_t child2_index = node.left_first + 1u;
        const Bvh2Node &child1 = nodes[child1_index], &child2 = nodes[child2_index];
        float dist1 = intersect_aabb(ray, child1.min_bounds, child1.max_bounds);
        float dist2 = intersect_aabb(ray, child2.min_bounds, child2.max_bounds);

        /* Swap child nodes so that the closest one comes first */
        if (dist1 > dist2) {
            std::swap(dist1, dist2), std::swap(child1_index, child2_index);
        }

        /* If we missed both child nodes */
        if (dist1 == BIG_F32) {
            /* Pop the node stack */
            if (stack_ptr == 0u) break;
            node_index = stack[--stack_ptr];
        } else {
            /* Continue with the closest child node */
            node_index = child1_index;
            /* Push the 2nd child onto the node stack if we hit it */
            if (dist2 != BIG_F32) stack[stack_ptr++] = child2_index;
        }
    }

    return Hit(min_t);
}

template <typename T>
Bvh2<T>::~Bvh2() {
    if (node_count > 0u) {
        delete[] nodes;
        delete[] prims;
        delete[] bounds;
        delete[] indices;
        delete[] gpu_nodes;
        node_count = 0u;
    }
}

/* Explicit template instantiations */
template class Bvh2<VoxelObject>;

}  // namespace tmt
