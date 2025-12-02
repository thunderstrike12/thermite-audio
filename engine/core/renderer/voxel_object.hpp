#pragma once

namespace tmt {

/* Represents a renderable voxel object. (used on the GPU) */
struct VoxelObject {
    /* World-space to local-space transformation matrix. */
    glm::mat4x4 world_to_local {};
    /* Size of the object in voxels. */
    glm::uvec3 size {};
};

}  // namespace tmt
