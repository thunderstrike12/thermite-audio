#include "post_process_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>

#include "engine.hpp"
#include "core/renderer/renderer.hpp"
#include "tools/player_data.hpp"

namespace tmt {

void PostProcessPipeline::init(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    /* Create auto exposure buffers */
    autox_partial_texture = bank.create_texture("Partial AutoX Buffer Texture", TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::R32Sfloat, { 256u, 256u })
                                .expect("failed to initialize partial autox buffer texture");
    autox_partial_image = bank.create_image("Partial AutoX Buffer Image", autox_partial_texture).expect("failed to initialize partial autox buffer image.");
    autox_tiny_texture = bank.create_texture("Tiny AutoX Buffer Texture", TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::R32Sfloat, { 16u, 16u })
                             .expect("failed to initialize tiny autox buffer texture");
    autox_tiny_image = bank.create_image("Tiny AutoX Buffer Image", autox_tiny_texture).expect("failed to initialize tiny autox buffer image.");
    autox_texture = bank.create_texture("Final AutoX Buffer Texture", TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::R32Sfloat, { 1u, 1u })
                        .expect("failed to initialize autox buffer texture");
    autox_image = bank.create_image("Final AutoX Buffer Image", autox_texture).expect("failed to initialize autox buffer image.");

    down_sampler = bank.create_sampler("Down Sampler", Filter::Linear, AddressMode::ClampToBorder).expect("failed to create down sampler.");
    up_sampler = bank.create_sampler("Up Sampler", Filter::Linear, AddressMode::ClampToEdge).expect("failed to create up sampler.");
}

void PostProcessPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    const glm::uvec2 shading_res = render_view.gpu_view.resolution;

    RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");

    { /* Auto-exposure */
        /* clang-format off */
        /* Initial 256x256 aliased gather */
        render_graph.add_compute_pass("AutoX Gather", "lighting/autox_gather.cs")
            .read(render_view.render_view_buffer)
            .read(render_view.lbuffer.image)
            .write(autox_partial_image)
            .group_size(8, 8)
            .work_size(256, 256);

        /* 256x256 to 16x16 downsampling using LDS */
        render_graph.add_compute_pass("AutoX Average", "lighting/autox_average.cs")
            .read(autox_partial_image)
            .write(autox_tiny_image)
            .read(down_sampler)
            .group_size(8, 8)
            .work_size(128, 128);

        /* Final 16x16 average and temporal response */
        render_graph.add_compute_pass("AutoX Final", "lighting/autox_final.cs")
            .read(render_view.render_view_buffer)
            .read(autox_tiny_image)
            .write(autox_image)
            .read(down_sampler)
            .push_constants(&settings.autox_response, 0u, sizeof(float))
            .group_size(8, 8)
            .work_size(8, 8);

        struct AutoXConstants {
            float key_value;
            float lum_min;
            float lum_max;
        } autox_constants;
        
        autox_constants.key_value = settings.autox_key_value;
        autox_constants.lum_min = settings.autox_lum_min;
        autox_constants.lum_max = settings.autox_lum_max;

        /* Apply exposure to luminance buffer */
        render_graph.add_compute_pass("AutoX Apply", "lighting/autox_apply.cs")
            .read(render_view.dbuffer.image)
            .write(render_view.lbuffer.image)
            .read(autox_image)
            .push_constants(&autox_constants, 0u, sizeof(AutoXConstants))
            .group_size(16, 8)
            .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
        /* clang-format on */
    }

    /* TAA Resolve */
    const uint32_t frame_flag = (render_view.frame_counter & 1) == 0;
    uint32_t taa_flag = engine.renderer.enable_taa ? 1u : 0u;
    /* clang-format off */
    render_graph.add_compute_pass("TAA Resolve", "taa_resolve.cs")
        .read(render_view.render_view_buffer)
        .read(engine.renderer.point_sampler)
        .read(engine.renderer.linear_sampler)
        .read(render_view.mbuffer.image)
        .read(render_view.dbuffer.image)
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
        render_graph.add_compute_pass("Luminance Threshold", "luminance_threshold.cs")
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
        render_graph.add_compute_pass("Bloom Downsample", "bloom/downsample.cs")
                    .read(render_view.render_view_buffer)
                    .read(down_sampler)
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
        render_graph.add_compute_pass("Bloom Upsample", "bloom/upsample.cs")
                    .read(render_view.render_view_buffer)
                    .read(up_sampler)
                    .write(render_view.tbuffer.images[curr_mip])
                    .read(render_view.tbuffer.images[curr_mip + 1])
                    .push_constants(&upsample_constants, 0u, sizeof(UpsampleConstants))
                    .group_size(16, 8)
                    .work_size(mip_w, mip_h);
        /* clang-format on */
    }

    /* Bloom Addition, Color Grading and Tonemapping */
    /* clang-format off */
    render_graph.add_compute_pass("Color Grading and Tonemapping", "cg_tonemap.cs")
        .read(render_view.lbuffer.image)
        .read(render_view.tbuffer.images[0])
        .write(render_view.get_render_image())
        .group_size(16, 8)
        .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
    /* clang-format on */
}

void PostProcessPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    bank.destroy(autox_partial_image);
    bank.destroy(autox_partial_texture);
    bank.destroy(autox_tiny_image);
    bank.destroy(autox_tiny_texture);
    bank.destroy(autox_image);
    bank.destroy(autox_texture);
    bank.destroy(down_sampler);
    bank.destroy(up_sampler);
}

}  // namespace tmt
