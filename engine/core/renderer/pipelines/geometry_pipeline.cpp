#include "geometry_pipeline.hpp"

#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>
#include <graphite/nodes/raster_node.hh>

#include "engine/engine.hpp"
#include "engine/core/renderer/render_view.hpp"
#include "engine/core/renderer/scene_view.hpp"
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

/* clang-format off */

/* Geometry display mode switch case macro. */
#define DISPLAY_MODE(ENUM, NAME, PATH) case DisplayMode::ENUM: \
    render_graph.add_compute_pass(NAME, PATH) \
        .read(render_view.render_view_buffer) \
        .read(scene_view.object_data) \
        .read(render_view.vbuffer.image) \
        .write(render_image) \
        .group_size(16, 8) \
        .work_size(render_res.x, render_res.y); \
    break;

void GeometryPipeline::enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view) {
    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();

    /* Enqueue the geometry compute pass */
    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    if (engine.renderer.display_mode != DisplayMode::STEPS) {
        render_graph.add_compute_pass("geometry pass", "geometry.cs")
            /* Render view */
            .read(render_view.render_view_buffer)
            /* Object buffers */
            .read(scene_view.bvh_nodes)
            .read(scene_view.object_indices)
            .read(scene_view.object_data)
            /* Visibility buffer */
            .write(render_view.vbuffer.image)
            /* Motion Vector buffer */
            .write(render_view.mbuffer.image)
            .group_size(16, 8)
            .work_size(render_res.x, render_res.y);
    } else {
        /* Debug visualization for visibility step count */
        render_graph.add_compute_pass("[debug] step count pass", "debug/steps.cs")
            /* Render view */
            .read(render_view.render_view_buffer)
            /* Object buffers */
            .read(scene_view.bvh_nodes)
            .read(scene_view.object_indices)
            .read(scene_view.object_data)
            /* Render target */
            .write(render_image)
            .group_size(16, 8)
            .work_size(render_res.x, render_res.y);
        return;
    }

    /* Debug visualizations */
    switch (engine.renderer.display_mode) {
        DISPLAY_MODE(VISIBILITY, "[debug] visibility pass", "debug/visibility.cs")
        DISPLAY_MODE(DEPTH, "[debug] depth pass", "debug/depth.cs")
        DISPLAY_MODE(NORMALS, "[debug] normals pass", "debug/normals.cs")
        DISPLAY_MODE(ALBEDO, "[debug] albedo pass", "debug/albedo.cs")
        case DisplayMode::MOTIONVECTORS:
            render_graph.add_compute_pass("[debug] motion vectors pass", "debug/motion_vectors.cs")
            .read(render_view.mbuffer.image)
            .write(render_image)
            .group_size(16, 8)
            .work_size(render_res.x, render_res.y);
            break;
        default: 
            break;
    }

    /* Depth transfer pass */
    RasterNode& transfer_pass = render_graph.add_raster_pass("depth transfer pass", "depth_transfer.vx", "depth_transfer.px")
        .topology(Topology::TriangleList)
        .read(render_view.render_view_buffer, ShaderStages::Pixel)
        .read(render_view.vbuffer.image, ShaderStages::Pixel)
        .read(scene_view.object_data, ShaderStages::Pixel)
        .load_op_depth(LoadOp::Clear) /* Clear the depth buffer */
        .depth_stencil(render_view.dbuffer.image, true, true)
        .raster_extent(render_res.x, render_res.y);
    transfer_pass.draw(NULL_BUFFER, 3u);
}

/* clang-format on */

}  // namespace tmt
