#pragma once

#include "engine/core/resource.hpp"
#include "engine/shared/svt64.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

/* Voxel scene resource node. */
struct VoxelSceneNode {
    /* 128 bit unique identifier. */
    uint64_t uuid[2] {};

    /* Voxel acceleration structure. */
    std::unique_ptr<Svt64> tree {};
    glm::uvec3 size {};

    /* Child nodes. */
    std::vector<VoxelSceneNode> children {};

    /* Returns true if this node is a voxel volume. */
    inline bool is_volume() const { return tree != nullptr; }
};

/* Voxel scene resource, loaded from a `.vengi` file. */
class VoxelScene : public tmt::FileResource {
   public:
    VoxelScene(tmt::IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;

    /* Voxel scene hierarchy. */
    VoxelSceneNode hierarchy {};
};

}  // namespace tmt
