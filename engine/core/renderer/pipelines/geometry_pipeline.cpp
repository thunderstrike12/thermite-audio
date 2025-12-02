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

    /* Create the objects buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Storage | BufferUsage::TransferDst, MAX_VOXEL_OBJECTS, sizeof(VoxelObject)); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to create objects buffer.\nreason: {}", r.unwrap_err().c_str());
        return;
    } else {
        object_buffer = r.unwrap();
    }
}

void GeometryPipeline::enqueue(RenderGraph& render_graph, Buffer render_view, RenderTarget render_target) {
    /* Capture all voxel renderers in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const VoxelRenderer>(entt::get<Transform>);

    /* Allocate space for all voxel objects */
    std::vector<VoxelObject> objects {};
    objects.reserve(min(group.size(), (size_t)MAX_VOXEL_OBJECTS));

    /* Iterate over all voxel renderers */
    for (auto&& [entity, renderer, transform] : group.each()) {
        /* Convert the entity to a voxel object */
        VoxelObject object {};
        object.world_to_local = glm::inverse(transform.get_world_matrix());
        object.size = renderer.size;
    }

    /* Update objects buffer */
    render_graph.upload_buffer(object_buffer, objects.data(), 0u, sizeof(VoxelObject) * objects.size());

    /* clang-format off */

    /* Enqueue the geometry compute pass */
    render_graph.add_compute_pass("geometry pass", "geometry.cs")
        .read(render_view)
        .read(object_buffer)
        .write(render_target)
        .group_size(16, 8)
        .work_size(engine.window.width, engine.window.height);

    /* clang-format on */
}

void GeometryPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();
    bank.destroy(object_buffer);
}

}  // namespace tmt
