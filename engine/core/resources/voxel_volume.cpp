#include "voxel_volume.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/render_graph.hh>

#include "engine/core/logger.hpp"

#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/tools/serializer/uuid.hpp"

namespace tmt {

namespace {

const VoxelSceneNode* find_model_by_uuid(const VoxelSceneNode& node, const UUID& uuid) {
    if (node.uuid == uuid) return &node;

    for (const VoxelSceneNode& child : node.children) {
        const VoxelSceneNode* return_node = find_model_by_uuid(child, uuid);
        if (return_node != nullptr) return return_node;
    }

    return nullptr;
}

}  // namespace

/* Recursively find the first model inside a voxel model hierarchy. */
const VoxelSceneNode* first_model(const VoxelSceneNode& parent) {
    if (parent.tree != nullptr) return &parent;

    for (const VoxelSceneNode& child : parent.children) {
        const VoxelSceneNode* model = first_model(child);
        if (model != nullptr) return model;
    }
    return nullptr;
}

VoxelVolume::VoxelVolume(const glm::uvec3& grid_size) : Base({}, {}) {
    RawVoxels raw_voxels { .w = grid_size.x, .h = grid_size.y, .d = grid_size.z };

    const size_t total_size = grid_size.x * grid_size.y * grid_size.z;
    raw_voxels.materials.resize(total_size, AIR_INDEX);
    raw_voxels.physics_data.resize(total_size, { EMPTY, 0 });

    /* Create and build the voxel acceleration structure */
    blas = std::make_unique<Svt64>();
    blas->build(raw_voxels);

    size = grid_size;
    uuid = UUIDGenerator::generate(); /* Generate a new UUID. */

    create_gpu_buffers();
}

VoxelVolume::VoxelVolume(const VoxelSceneNode& node) : Base({}, {}) {
    /* Copy the voxel data from the model */
    blas = std::make_unique<Svt64>(*node.tree);
    size = node.size;
    uuid = node.uuid;
    name = node.name;

    create_gpu_buffers();
}

VoxelVolume::VoxelVolume(const ResourceRef<VoxelVolume>& volume, const UUID& new_uuid) : Base({}, {}) {
    /* Copy the voxel data from the model */
    blas = std::make_unique<Svt64>(*volume->blas);
    size = volume->size;
    uuid = new_uuid;
    name = volume->name;

    create_gpu_buffers();
}

bool VoxelVolume::load() {
    const VoxelSceneNode* model = nullptr;

    if (uuid == NULL_UUID) {
        for (VoxelSceneNode& root_node : file_resource->root_nodes) {
            model = first_model(root_node);

            if (model != nullptr) break;  // If a first node was found in a root node, then we exit the loop.
        }
    } else {
        for (VoxelSceneNode& root_node : file_resource->root_nodes) {
            model = find_model_by_uuid(root_node, uuid);

            if (model != nullptr) break;  // If the node with the UUID was found in the root node, then we exit the loop.
        }
    }

    if (model == nullptr) {
        Log::error("Failed to get model for VoxelVolume.");
        return false;
    }

    /* Copy the voxel data from the model */
    blas = std::make_unique<Svt64>(*model->tree);
    size = model->size;
    name = model->name;
    uuid = model->uuid;

    create_gpu_buffers();

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

bool VoxelVolume::fallback(FallbackReason) {
    const VoxelSceneNode* model = nullptr;

    for (VoxelSceneNode& root_node : file_resource->root_nodes) {
        model = first_model(root_node);

        if (model != nullptr) break;  // If a first node was found in a root node, then we exit the loop.
    }

    if (model == nullptr) {
        Log::error("Failed to get model for VoxelVolume.");
        return false;
    }

    /* Copy the voxel data from the model */
    blas = std::make_unique<Svt64>(*model->tree);
    size = model->size;
    name = model->name;
    uuid = model->uuid;

    create_gpu_buffers();

    return true;
}

void VoxelVolume::update_if_dirty() {
    if (is_dirty == false) return;
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Mark this volume as no longer dirty */
    is_dirty = false;

    if (blas->voxel_count >= blas_voxels_capacity || blas->node_count >= blas_nodes_capacity) {
        /* Resize the BLAS buffers */
        blas_nodes_capacity = blas->nodes_capacity;
        blas_voxels_capacity = blas->voxels_capacity;
        bank.resize_buffer(blas_nodes, blas_nodes_capacity, sizeof(Svt64Node));
        bank.resize_buffer(blas_voxels, blas_voxels_capacity, sizeof(MaterialIndex));
        Log::info("Resized SVT64 buffers.");
    }

    /* Re-upload voxel data */
    bank.upload_buffer(blas_voxels, blas->materials, 0u, blas->voxel_count * sizeof(MaterialIndex));
    bank.upload_buffer(blas_nodes, blas->nodes, 0u, blas->node_count * sizeof(Svt64Node));
    bank.upload_buffer(blas_palette, &blas->palette, 0u, sizeof(MaterialPalette));
}

void VoxelVolume::create_gpu_buffers() {
    /* Create GPU buffers for the voxel data */
    /* TODO: These buffers need to scale when the voxel data is modified at run-time! */
    VRAMBank& bank = engine.renderer.vram_bank();
    const BufferUsage storage = BufferUsage::Storage | BufferUsage::TransferDst;
    blas_nodes_capacity = blas->nodes_capacity;
    blas_voxels_capacity = blas->voxels_capacity;
    blas_nodes = bank.create_buffer("BLAS Nodes Buffer", storage, blas_nodes_capacity, sizeof(Svt64Node)).expect("failed to create tree nodes buffer.");
    blas_voxels = bank.create_buffer("BLAS Voxels Buffer", storage, blas_voxels_capacity, sizeof(MaterialIndex)).expect("failed to create voxel data buffer.");
    blas_palette = bank.create_buffer("BLAS Palette Buffer", storage, sizeof(MaterialPalette)).expect("failed to create material palette buffer.");

    /* Upload the voxel data into the GPU buffers */
    /* TODO: These buffers need to be updated when voxel data is modified at run-time! */
    bank.upload_buffer(blas_nodes, blas->nodes, 0u, blas->node_count * sizeof(Svt64Node));
    bank.upload_buffer(blas_voxels, blas->materials, 0u, blas->voxel_count * sizeof(MaterialIndex));
    bank.upload_buffer(blas_palette, &blas->palette, 0u, sizeof(MaterialPalette));
}

}  // namespace tmt
