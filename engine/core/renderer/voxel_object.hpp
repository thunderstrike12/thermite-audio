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
    uint8_t padding : 8;

    /* Get the axis-aligned bounding box of this voxel object. */
    inline Aabb aabb() const {
        const glm::vec3 center = glm::vec3(local_to_world[3]);
        const glm::vec3 unit_extent = glm::vec3(size) * UNITS_PER_VOXEL;
        const glm::vec3 x_extent = glm::abs(glm::vec3(local_to_world[0]) * unit_extent);
        const glm::vec3 y_extent = glm::abs(glm::vec3(local_to_world[1]) * unit_extent);
        const glm::vec3 z_extent = glm::abs(glm::vec3(local_to_world[2]) * unit_extent);
        const glm::vec3 extent = x_extent + y_extent + z_extent;
        return Aabb(center - extent, center + extent);
    }

    /* Intersect the voxel object with a ray. */
    float intersect(const Ray&) const {
        // TODO: Implement the ray intersection function!
        return 0.0f;
    }
};

}  // namespace tmt
