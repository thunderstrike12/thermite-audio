#pragma once

#include "shared/aabb.hpp"

namespace tmt {

struct Ray;

/* Represents a renderable voxel object. (used on the GPU) */
struct alignas(16) VoxelObject {
    /* World-space to local-space transformation matrix. */
    glm::mat4 world_to_local {};
    /* Local-space to world-space transformation matrix. */
    glm::mat4 local_to_world {};

    /* Size of the object in voxels. */
    glm::uvec3 size {};
    /* Depth of the voxel tree structure. */
    float rcp_tree_width = 0.0f;

    /* Bindless resource handles. */
    uint32_t blas_handle = 0u;
    uint32_t voxels_handle = 0u;
    uint32_t palette_handle = 0u;

    /* Padding */
    uint32_t : 32;

    /* Get the axis-aligned bounding box of this voxel object. */
    inline Aabb aabb() const {
        const glm::vec3 center = glm::vec3(local_to_world[3]);
        const glm::vec3 unit_extent = glm::vec3(size) * UNITS_PER_VOXEL * 0.5f;
        const glm::vec3 extent = glm::mat3(glm::abs(glm::vec3(local_to_world[0])), glm::abs(glm::vec3(local_to_world[1])), glm::abs(glm::vec3(local_to_world[2]))) * unit_extent;
        return Aabb(center - extent, center + extent);
    }

    /* Intersect the voxel object with a ray. */
    float intersect(const Ray&) const {
        // TODO: Implement the ray intersection function!
        return 0.0f;
    }
};

}  // namespace tmt
