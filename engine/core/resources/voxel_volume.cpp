#include "voxel_volume.hpp"

#include <graphite/vram_bank.hh>

#include "engine/core/logger.hpp"

#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

/* Recursively find the first model inside a voxel model hierarchy. */
const VoxelSceneNode* first_model(const VoxelSceneNode* parent) {
    if (parent->tree != nullptr) return parent;
    for (uint32_t i = 0u; i < parent->children.size(); ++i) {
        const VoxelSceneNode* model = first_model(&parent->children[i]);
        if (model != nullptr) return model;
    }
    return nullptr;
}

bool VoxelVolume::load() {
    /* TEMP: It's currently just grabbing the first model node */
    const VoxelSceneNode* model = first_model(&file_resource->hierarchy);

    /* Copy the voxel data from the model */
    blas = std::make_unique<Svt64>(*model->tree.get());
    size = model->size;

    /* Create GPU buffers for the voxel data */
    /* TODO: These buffers need to scale when the voxel data is modified at run-time! */
    VRAMBank& bank = engine.renderer.vram_bank();
    const BufferUsage storage = BufferUsage::Storage | BufferUsage::TransferDst;
    blas_nodes = bank.create_buffer("BLAS Nodes Buffer", storage, blas->node_count, sizeof(Svt64Node)).expect("failed to create tree nodes buffer.");
    blas_voxels = bank.create_buffer("BLAS Voxels Buffer", storage, blas->voxel_count, sizeof(MaterialIndex)).expect("failed to create voxel data buffer.");
    blas_palette = bank.create_buffer("BLAS Palette Buffer", storage, 255u, sizeof(Material)).expect("failed to create material palette buffer.");

    /* Upload the voxel data into the GPU buffers */
    /* TODO: These buffers need to be updated when voxel data is modified at run-time! */
    bank.upload_buffer(blas_nodes, blas->nodes, 0u, blas->node_count * sizeof(Svt64Node));
    bank.upload_buffer(blas_voxels, blas->materials, 0u, blas->voxel_count * sizeof(MaterialIndex));
    bank.upload_buffer(blas_palette, &blas->palette, 0u, 255u * sizeof(Material));

    return true;
}

void VoxelVolume::unload() {
    /* Destroy the (bindless) GPU buffers */
    engine.renderer.destroy(blas_nodes);
    engine.renderer.destroy(blas_voxels);
    engine.renderer.destroy(blas_palette);

    /* Set the BLAS to null */
    blas = nullptr;
    size = glm::uvec3(0u);
}

}  // namespace tmt
