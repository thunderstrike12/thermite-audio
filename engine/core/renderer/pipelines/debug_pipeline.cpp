#include "debug_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>

#include "engine.hpp"
// #include "core/window.hpp"
#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

void DebugPipeline::init(GPUAdapter& gpu) {
    /* Get VRAM Bank */
    VRAMBank& bank = gpu.get_vram_bank();

    /* Initialize the Line Buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Vertex | BufferUsage::TransferDst, MAX_DEBUG_POINTS, sizeof(GpuPoint)); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialise line buffer.\nreason: {}", r.unwrap_err().c_str());
        return;
    } else
        point_buffer = r.unwrap();
}

void DebugPipeline::on_engine_update(const FrameData& time) {
    if (!timed_points.empty()) {
        for (auto& p : timed_points) p.time -= time.delta_time;

        std::erase_if(timed_points, [](const DebugPoint& p) { return p.time <= 0.0f; });
    }

    num_points = timed_points.size() + persistent_points.size();
}

void DebugPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();

    /* Upload Point Buffer */
    if (num_points > 0) {
        std::vector<GpuPoint> packed {};

        for (int i = 0; i < timed_points.size(); i++) {
            packed.push_back(timed_points[i].point);
        }
        for (int j = 0; j < persistent_points.size(); j++) {
            packed.push_back(persistent_points[j].point);
        }

        render_graph.upload_buffer(point_buffer, packed.data(), 0, sizeof(GpuPoint) * num_points);
    }

    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    RasterNode& line_pass = render_graph.add_raster_pass("debug line pass", "debug_line.vx", "debug_line.px")
                                .topology(Topology::LineList)
                                .attribute(AttrFormat::XYZ32_SFloat)  // Position
                                .attribute(AttrFormat::XYZ32_SFloat)  // Color
                                .read(render_view.render_view_buffer, ShaderStages::Vertex)
                                .attach(render_image)
                                .raster_extent(render_res.x, render_res.y);
    line_pass.draw(point_buffer, num_points);

    // Update Points
    num_points -= persistent_points.size();
    persistent_points.clear();
}

void DebugPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    bank.destroy(point_buffer);
}

void DebugPipeline::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color, const float time) {
    if (num_points > MAX_DEBUG_POINTS - 2) {
        Log::error(Log::Scope::RENDERER, "max amount of lines reached. Consider increasing MAX_DEBUG_POINTS, or draw less lines.");
        return;
    }

    // Timed Lines
    if (time > 0.0f) {
        timed_points.push_back({{start, color}, time});
        timed_points.push_back({{end, color}, time});
    } else {  // Persistent Lines
        persistent_points.push_back({{start, color}, time});
        persistent_points.push_back({{end, color}, time});
    }
    num_points += 2;
}

}  // namespace tmt