#include "geometry_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/logger.hpp"
#include "core/window.hpp"

#include "core/components/transform.hpp"
#include "core/components/voxel_renderer.hpp"
#include "core/renderer/voxel_object.hpp"

namespace tmt {

void GeometryPipeline::init(GPUAdapter& gpu) {
    /* Get VRAM Bank */
    VRAMBank& bank = gpu.get_vram_bank();

    /* Create GPU resources */
    const BufferUsage usage = BufferUsage::Storage | BufferUsage::TransferDst;
    bvh_nodes = bank.create_buffer(usage, MAX_VOXEL_OBJECTS * 2u + 1u, sizeof(AilaLaineNode)).expect("failed to create bvh nodes buffer.");
    object_indices = bank.create_buffer(usage, MAX_VOXEL_OBJECTS, sizeof(uint32_t)).expect("failed to create object indices buffer.");
    object_data = bank.create_buffer(usage, MAX_VOXEL_OBJECTS, sizeof(VoxelObject)).expect("failed to create object data buffer.");
}

void GeometryPipeline::enqueue(RenderGraph& render_graph, Buffer render_view, RenderTarget render_target) {
    /* Capture all voxel renderers in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const VoxelRenderer>(entt::get<Transform>);

    /* Allocate space for all voxel objects */
    std::vector<VoxelObject> objects {};
    objects.reserve(std::min(group.size(), (size_t)MAX_VOXEL_OBJECTS));

    /* Iterate over all voxel renderers */
    for (auto&& [entity, renderer, transform] : group.each()) {
        /* Convert the entity to a voxel object */
        VoxelObject object {};
        object.local_to_world = transform.get_world_matrix();
        object.world_to_local = glm::inverse(object.local_to_world);
        object.size = renderer.size;
        objects.push_back(std::move(object));
    }

    /* Build a BVH over the scene */
    bvh.build(objects.data(), (uint32_t)objects.size());

    /* Upload the object buffers */
    render_graph.upload_buffer(bvh_nodes, bvh.gpu_nodes, 0u, bvh.node_count * sizeof(AilaLaineNode));
    render_graph.upload_buffer(object_indices, bvh.indices, 0u, bvh.prim_count * sizeof(uint32_t));
    render_graph.upload_buffer(object_data, bvh.prims, 0u, bvh.prim_count * sizeof(VoxelObject));

    /* clang-format off */

    /* Enqueue the geometry compute pass */
    render_graph.add_compute_pass("geometry pass", "geometry.cs")
        /* Render view */
        .read(render_view)
        /* Object buffers */
        .read(bvh_nodes)
        .read(object_indices)
        .read(object_data)
        /* Render target */
        .write(render_target)
        .group_size(16, 8)
        .work_size(engine.window.width, engine.window.height);

    /* clang-format on */
}

void GeometryPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();
    bank.destroy(bvh_nodes);
    bank.destroy(object_indices);
    bank.destroy(object_data);
}

}  // namespace tmt
