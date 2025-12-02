#pragma once

namespace tmt {

/* Voxel Renderer component used to visualize voxel data. */
struct VoxelRenderer {
    /* Size of the voxel data. (TODO: move this into the run-time resource) */
    glm::uvec3 size {};
};

}  // namespace tmt
