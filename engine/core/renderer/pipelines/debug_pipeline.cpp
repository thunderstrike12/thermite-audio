#include "debug_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>

#include "engine.hpp"
#include "core/window.hpp"
#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

void DebugPipeline::init(GPUAdapter& gpu) {
    /* Get VRAM Bank */
    VRAMBank& bank = gpu.get_vram_bank();

    /* Initialize the Line Buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Vertex | BufferUsage::TransferDst, MAX_DEBUG_POINTS, sizeof(DebugPoint)); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialise line buffer.\nreason: {}", r.unwrap_err().c_str());
        return;
    } else
        point_buffer = r.unwrap();
}

void DebugPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();

    /* Update Point Buffer */
    num_points = (u32)debug_points.size();
    if (num_points > 0) {
        render_graph.upload_buffer(point_buffer, debug_points.data(), 0, sizeof(DebugPoint) * num_points);
    }
    debug_points.clear();

    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    RasterNode& line_pass = render_graph.add_raster_pass("debug line pass", "debug_line.vx", "debug_line.px")
                                .topology(Topology::LineList)
                                .attribute(AttrFormat::XYZ32_SFloat)  // Position
                                .attribute(AttrFormat::XYZ32_SFloat)  // Color
                                .read(render_view.render_view_buffer, ShaderStages::Vertex)
                                .attach(render_image)
                                .raster_extent(render_res.x, render_res.y);
    line_pass.draw(point_buffer, num_points);
}

void DebugPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    bank.destroy(point_buffer);
}

void DebugPipeline::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color) {
    if (debug_points.size() <= MAX_DEBUG_POINTS - 2) {
        debug_points.push_back({start, color});
        debug_points.push_back({end, color});
    }
}

}  // namespace tmt