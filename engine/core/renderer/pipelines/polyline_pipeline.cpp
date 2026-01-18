#include "polyline_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>

#include "engine.hpp"
#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

void PolylinePipeline::init(GPUAdapter& gpu) {
    /* Initialize the polyline vertex buffer */
    VRAMBank& bank = gpu.get_vram_bank();
    line_buffer =
        bank.create_buffer("Polyline Vertex Buffer", BufferUsage::Vertex | BufferUsage::TransferDst, MAX_POLYLINES, sizeof(PolylineSegment)).expect("failed to create polyline vertex buffer.");
}

void PolylinePipeline::on_engine_update(const FrameData& time) {
    /* Remove any timed line segments which have run out of time */
    if (timed_lines.empty() == false) {
        for (TimedPolylineSegment& p : timed_lines) p.timer -= time.delta_time;
        std::erase_if(timed_lines, [](const TimedPolylineSegment& p) { return p.timer <= 0.0f; });
    }

    /* Update the number of active line segments */
    line_segment_count = (uint32_t)timed_lines.size() + (uint32_t)immediate_lines.size();
}

void PolylinePipeline::enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view) {
    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();

    /* Collect and upload polyline segments */
    if (line_segment_count > 0u) {
        /* Create a vector of polyline segments */
        std::vector<PolylineSegment> segments {};
        segments.reserve(line_segment_count);

        /* Collect all the segments */
        for (const TimedPolylineSegment& segment : timed_lines) segments.push_back(segment.line);
        for (const PolylineSegment& segment : immediate_lines) segments.push_back(segment);

        /* Upload all the segments */
        render_graph.upload_buffer(line_buffer, segments.data(), 0, sizeof(PolylineSegment) * segments.size());
    } else
        return;

    /* clang-format off */

    /* Depth transfer pass */
    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    RasterNode& transfer_pass = render_graph.add_raster_pass("depth transfer pass", "depth_transfer.vx", "depth_transfer.px")
        .topology(Topology::TriangleList)
        .read(render_view.render_view_buffer, ShaderStages::Pixel)
        .read(render_view.vbuffer.image, ShaderStages::Pixel)
        .read(scene_view.object_data, ShaderStages::Pixel)
        .load_op_depth(LoadOp::Clear) /* Clear the depth buffer */
        .depth_stencil(render_view.dbuffer.image, true, true)
        .raster_extent(render_res.x, render_res.y);
    transfer_pass.draw(NULL_BUFFER, 3u);

    /* Polyline render pass */
    RasterNode& line_pass = render_graph.add_raster_pass("polyline pass", "polyline.vx", "polyline.px")
        .topology(Topology::TriangleList)
        .attribute(AttrFormat::XYZ32_SFloat)  /* Begin */
        .attribute(AttrFormat::XYZ32_SFloat)  /* End */
        .attribute(AttrFormat::XYZW32_SFloat) /* Color */
        .attribute(AttrFormat::X32_SFloat)    /* Width */
        .input_rate(VertexInputRate::Instance)
        .alpha_blending(true)
        .read(render_view.render_view_buffer, ShaderStages::Vertex)
        .attach(render_image)
        .depth_stencil(render_view.dbuffer.image)
        .raster_extent(render_res.x, render_res.y);
    line_pass.draw(line_buffer, 6u, 0u, line_segment_count, 0u);

    /* clang-format on */

    /* Subtract the number of immediate lines from the line segment count */
    line_segment_count -= (uint32_t)immediate_lines.size();
    immediate_lines.clear();
}

void PolylinePipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();
    bank.destroy(line_buffer);
}

}  // namespace tmt
