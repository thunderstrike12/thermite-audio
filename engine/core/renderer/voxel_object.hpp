#pragma once

#include "engine/shared/aabb.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/resources/voxel_volume.hpp"

namespace tmt {

struct Ray;

/* Represents a renderable voxel object. (used on the GPU) */
struct alignas(16) GpuVoxelObject {
    /* World-space to local-space transformation matrix. */
    glm::mat4 world_to_local {};
    /* Local-space to world-space transformation matrix. */
    glm::mat4 local_to_world {};

    /* Size of the object in voxels. */
    glm::uvec3 size {};
    /* Reciprocal of the width of the voxel tree structure. */
    float rcp_tree_width = 0.0f;
    /* Depth of the voxel tree structure. */
    uint32_t tree_depth = 0u;

    /* Bindless resource handles. */
    uint32_t blas_handle = 0xFFFFFFFFu;
    uint32_t voxels_handle = 0xFFFFFFFFu;
    uint32_t palette_handle = 0xFFFFFFFFu;

    /* Object flags, 0b = outlined. */
    uint32_t object_flags = 0u;

    /* Padding. */
    glm::uvec3 padding {};
};

struct VoxelObject {
    /* World-space to local-space transformation matrix. */
    glm::mat4 world_to_local {};
    /* Local-space to world-space transformation matrix. */
    glm::mat4 local_to_world {};

    /* Size of the object in voxels. */
    glm::uvec3 size {};
    /* Reciprocal of the width of the voxel tree structure. */
    float rcp_tree_width = 0.0f;

    VoxelVolume* volume {};

    /* Get the axis-aligned bounding box of this voxel object. */
    inline Aabb aabb() const {
        const glm::vec3 center = glm::vec3(local_to_world[3]);
        const glm::vec3 unit_extent = glm::vec3(size) * UNITS_PER_VOXEL * 0.5f;
        const glm::vec3 extent = glm::mat3(glm::abs(glm::vec3(local_to_world[0])), glm::abs(glm::vec3(local_to_world[1])), glm::abs(glm::vec3(local_to_world[2]))) * unit_extent;
        return Aabb(center - extent, center + extent);
    }

    /* Intersect the voxel object with a ray. */
    Hit intersect(Ray ray, const float tmax) const;
};

}  // namespace tmt
