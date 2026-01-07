#include "scene_view.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/render_graph.hh>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#undef min
#undef max

namespace tmt {

void SceneView::init() {
    /* Get the VRAM bank */
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Create GPU resources */
    const BufferUsage usage = BufferUsage::Storage | BufferUsage::TransferDst;
    bvh_nodes = bank.create_buffer("BVH Nodes Buffer", usage, MAX_VOXEL_OBJECTS * 2u + 1u, sizeof(AilaLaineNode)).expect("failed to create bvh nodes buffer.");
    object_indices = bank.create_buffer("Object Indices Buffer", usage, MAX_VOXEL_OBJECTS, sizeof(uint32_t)).expect("failed to create object indices buffer.");
    object_data = bank.create_buffer("Object Data Buffer", usage, MAX_VOXEL_OBJECTS, sizeof(VoxelObject)).expect("failed to create object data buffer.");
}

void SceneView::update(RenderGraph& render_graph) {
    /* Capture all voxel renderers in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const VoxelRenderer>(entt::get<Transform>);

    /* Allocate space for all voxel objects */
    std::vector<VoxelObject> objects {};
    objects.reserve(std::min(group.size(), (size_t)MAX_VOXEL_OBJECTS));

    /* Iterate over all voxel renderers */
    for (auto&& [entity, renderer, transform] : group.each()) {
        /* Respect the object limit */
        if (objects.size() >= (size_t)MAX_VOXEL_OBJECTS) break;

        /* Don't render objects with a zero scale or null resource */
        if (glm::any(glm::equal(transform.get_world_scale(), glm::vec3(0.0f))) || renderer.resource == nullptr) continue;

        /* Convert the entity to a voxel object */
        VoxelObject object {};
        object.local_to_world = transform.get_world_matrix();
        object.world_to_local = glm::inverse(object.local_to_world);
        object.size = renderer.resource->size;
        object.rcp_tree_width = 1.0f / powf(4.0f, (float)renderer.resource->blas->depth);
        object.tree_depth = renderer.resource->blas->depth;
        object.blas_handle = renderer.resource->blas_nodes.get_index();
        object.voxels_handle = renderer.resource->blas_voxels.get_index();
        object.palette_handle = renderer.resource->blas_palette.get_index();
        objects.push_back(std::move(object));
    }

    /* Build a BVH over the scene */
    bvh.build(objects.data(), (uint32_t)objects.size());

    /* Upload the BVH buffers */
    render_graph.upload_buffer(bvh_nodes, bvh.gpu_nodes, 0u, bvh.node_count * sizeof(AilaLaineNode));
    render_graph.upload_buffer(object_indices, bvh.indices, 0u, bvh.prim_count * sizeof(uint32_t));
    render_graph.upload_buffer(object_data, bvh.prims, 0u, bvh.prim_count * sizeof(VoxelObject));
}

void SceneView::deinit() {
    /* Get the VRAM bank */
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Destroy GPU resources */
    bank.destroy(bvh_nodes);
    bank.destroy(object_indices);
    bank.destroy(object_data);
}

}  // namespace tmt
