#pragma once

#include <memory>

#include "engine/core/resources/voxel_volume.hpp"

namespace tmt {

/* Voxel Renderer component used to visualize voxel data. */
struct VoxelRenderer {
    std::shared_ptr<VoxelVolume> resource {};
};

}  // namespace tmt
