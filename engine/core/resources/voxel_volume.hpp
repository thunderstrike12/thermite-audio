#pragma once

#include <graphite/resources/handle.hh>

#include "voxel_scene.hpp"

#include "engine/core/reflection.hpp"

namespace tmt {

/* Voxel volume run-time resource, created from a Voxel model resource. */
class VoxelVolume : public tmt::RuntimeResource<VoxelScene, UUID> {
    using Base = tmt::RuntimeResource<VoxelScene, UUID>;

   public:
    // Constructor for temporary VoxelVolume's that aren't managed by the resource system (used in the voxel editor).
    VoxelVolume(const glm::uvec3& grid_size);
    VoxelVolume(const VoxelSceneNode& node);
    VoxelVolume(const ResourceRef<VoxelVolume>& volume, const UUID& new_uuid);

    VoxelVolume(const std::shared_ptr<VoxelScene>& file_resource) : Base(file_resource, NULL_UUID) {}
    VoxelVolume(const std::shared_ptr<VoxelScene>& file_resource, const UUID& uuid) : Base(file_resource, uuid), uuid { uuid } {}
    ~VoxelVolume() override { VoxelVolume::unload(); }

    bool load() override;
    void unload() override;

    bool fallback(FallbackReason reason) override;

    /* Updates the GPU buffers of this voxel volume if dirty. (note: this should only be called from the renderer) */
    void update_if_dirty();

    /* Mark this voxel volume as dirty, meaning it needs to be re-uploaded to the GPU. */
    void set_dirty() { is_dirty = true; }

    /* Human-readable name */
    std::string name {};

    /* 128 bit unique identifier. */
    UUID uuid { NULL_UUID };

    /* Voxel acceleration structure. */
    std::unique_ptr<Svt64> blas {};
    glm::uvec3 size {};

    /* Voxel acceleration structure buffers. */
    Buffer blas_nodes {}, blas_voxels {}, blas_palette {};

   private:
    void create_gpu_buffers();

    uint32_t blas_nodes_capacity = 0u, blas_voxels_capacity = 0u;
    bool is_dirty = false;
};

}  // namespace tmt

TMT_OBJECT_INSPECT_EMPTY(tmt::VoxelVolume);
TMT_OBJECT_SERIALIZE_EMPTY(tmt::VoxelVolume);