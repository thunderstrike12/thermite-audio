#pragma once
#include <glm/glm.hpp>
#include "engine/core/reflection.hpp"

#include <memory>

#include "engine/core/resources/voxel_volume.hpp"

namespace tmt {

/* Voxel Renderer component used to visualize voxel data. */
struct VoxelRenderer {
    ResourceRef<VoxelVolume> resource {};
};

}  // namespace tmt

TMT_COMPONENT(tmt::VoxelRenderer, "Voxel Renderer", (resource));