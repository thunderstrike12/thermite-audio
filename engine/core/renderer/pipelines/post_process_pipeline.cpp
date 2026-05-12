#include "post_process_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>

#include "engine.hpp"
#include "core/renderer/renderer.hpp"
#include "tools/player_data.hpp"
#include "core/input/input.hpp"

namespace tmt {

void PostProcessPipeline::init(GPUAdapter&) {}

void PostProcessPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    const glm::uvec2 shading_res = render_view.gpu_view.resolution;

    RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");
    const uint32_t frame_flag = (render_view.frame_counter & 1) == 0;

    /* TAA Resolve */
    uint32_t taa_flag = engine.renderer.enable_taa ? 1u : 0u;
    /* clang-format off */
    render_graph.add_compute_pass("taa resolve", "taa_resolve.cs")
        .read(render_view.render_view_buffer)
        .read(engine.renderer.point_sampler)
        .read(engine.renderer.linear_sampler)
        .read(render_view.mbuffer.image)
        .read(frame_flag ? render_view.depth_image : render_view.prev_depth_image)
        .read(frame_flag ? render_view.stencil_image : render_view.prev_stencil_image)
        .read(frame_flag ? render_view.prev_stencil_image : render_view.stencil_image)
        .write(render_view.lbuffer.image)
        .write(frame_flag ? render_view.hbuffer1.image : render_view.hbuffer2.image)
        .read(frame_flag ? render_view.hbuffer2.image : render_view.hbuffer1.image)
        .push_constants(&taa_flag, 0, sizeof(uint32_t))
        .group_size(16, 8)
        .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
    /* clang-format on */

    { /* Threshold the Luminance buffer */
        struct ThresholdConstants {
            float threshold;
            float trail;
        } threshold_constants;

        threshold_constants.threshold = settings.bloom_threshold;
        threshold_constants.trail = settings.bloom_trail;

        /* clang-format off */
        render_graph.add_compute_pass("luminance threshold", "luminance_threshold.cs")
                    .read(render_view.lbuffer.image)
                    .write(render_view.tbuffer.images[0])
                    .push_constants(&threshold_constants, 0u, sizeof(ThresholdConstants))
                    .group_size(16, 8)
                    .work_size(shading_res.x, shading_res.y);
        /* clang-format on */
    }

    for (uint32_t curr_mip = 1; curr_mip < render_view.tbuffer.meta.mips; curr_mip++) {
        const uint32_t mip_w = shading_res.x >> curr_mip;
        const uint32_t mip_h = shading_res.y >> curr_mip;

        /* clang-format off */
        render_graph.add_compute_pass("bloom downsample", "bloom/downsample.cs")
                    .read(render_view.render_view_buffer)
                    .read(engine.renderer.down_sampler)
                    .write(render_view.tbuffer.images[curr_mip])
                    .read(render_view.tbuffer.images[curr_mip - 1])
                    .push_constants(&curr_mip, 0u, sizeof(uint32_t))
                    .group_size(16, 8)
                    .work_size(mip_w, mip_h);
        /* clang-format on */
    }

    for (int32_t curr_mip = (int32_t)render_view.tbuffer.meta.mips - 2; curr_mip >= 0; curr_mip--) {
        const uint32_t mip_w = shading_res.x >> curr_mip;
        const uint32_t mip_h = shading_res.y >> curr_mip;

        struct UpsampleConstants {
            uint32_t curr_mip_level;
            float bloom_radius;
        } upsample_constants;

        upsample_constants.curr_mip_level = curr_mip;
        upsample_constants.bloom_radius = settings.bloom_radius;

        /* clang-format off */
        render_graph.add_compute_pass("bloom upsample", "bloom/upsample.cs")
                    .read(render_view.render_view_buffer)
                    .read(engine.renderer.up_sampler)
                    .write(render_view.tbuffer.images[curr_mip])
                    .read(render_view.tbuffer.images[curr_mip + 1])
                    .push_constants(&upsample_constants, 0u, sizeof(UpsampleConstants))
                    .group_size(16, 8)
                    .work_size(mip_w, mip_h);
        /* clang-format on */
    }

    /* Bloom Addition, Color Grading and Tonemapping */
    /* clang-format off */
    render_graph.add_compute_pass("color grading and tonemapping", "cg_tonemap.cs")
        .read(render_view.lbuffer.image)
        .read(render_view.tbuffer.images[0])
        .write(render_view.get_render_image())
        .group_size(16, 8)
        .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
    /* clang-format on */
}

void PostProcessPipeline::deinit(GPUAdapter&) {}

}  // namespace tmt
