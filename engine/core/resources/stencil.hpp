#pragma once

#include "engine/core/resources/voxel_scene.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

/* Voxel volume run-time resource, created from a Voxel model resource. */
class Stencil : public tmt::RuntimeResource<VoxelScene> {
   public:
    Stencil(const std::shared_ptr<VoxelScene>& file_resource) : RuntimeResource<VoxelScene>(file_resource) {}
    ~Stencil() { unload(); }

    bool load() override;
    void unload() override;

    glm::uvec3 size {};
    std::vector<char> data {};

    char get_voxel(const uint32_t x, const uint32_t y, const uint32_t z) const {
        if (x >= size.x || y >= size.y || z >= size.z) return 0;
        const uint32_t idx = x + size.x * y + size.x * size.y * z;
        return data[idx];
    }

   private:
    void copy_recursive(const uint32_t node_id, const glm::ivec3 node_pos, const uint32_t node_scale, const Svt64* tree);
};

}  // namespace tmt