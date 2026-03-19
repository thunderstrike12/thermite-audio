#include "di_pipeline.hpp"

#include <graphite/render_graph.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/vram_bank.hh>
#include <graphite/nodes/compute_node.hh>

#include "engine/engine.hpp"
#include "engine/core/renderer/render_view.hpp"
#include "engine/core/renderer/scene_view.hpp"
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

/* Divide two numbers, rounding up. */
template <typename T>
T div_up(const T x, const T y) {
    return (x + y - 1) / y;
}

/* Divide two numbers, rounding up. */
glm::uvec2 div_up(const glm::uvec2 a, const uint32_t ax, const uint32_t ay) {
    return glm::uvec2(div_up(a.x, ax), div_up(a.y, ay));
}

void DiPipeline::init(GPUAdapter&) {}

/* clang-format off */

void DiPipeline::enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view) {
    /* Only run this pass if it's outputs are actually used */
    if (engine.renderer.display_mode != DisplayMode::ILLUMINANCE && engine.renderer.display_mode != DisplayMode::CACHE && engine.renderer.display_mode != DisplayMode::DEFAULT) return;

    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();
    const BindHandle diffuse_image = render_view.lbuffer.images[0];
    const BindHandle raw_specular_image = render_view.raw_spec_buffer.image;
    const BindHandle specular_image = render_view.spec_buffer.image;
    const glm::uvec2 render_res = render_view.gpu_view.resolution;

    if (cache_init == false) {
        /* Cache init pass */
        render_graph.add_compute_pass("cache init pass", "lighting/cache_init.cs")
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .group_size(128)
            .work_size(CACHE_SIZE);

        cache_init = true;
    } else {
        /* Cache eviction pass (amortize over 8 frames) */
        render_graph.add_compute_pass("cache eviction pass", "lighting/cache_evict.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .group_size(128)
            .work_size(div_up(CACHE_SIZE, 8u));
    }

    { /* Shading resolution based on shading rate */
        glm::uvec2 shading_res = render_view.gpu_view.resolution;
        if (render_view.get_shading_rate_di() == ShadingRate::HALF_RATE) shading_res = div_up(render_view.gpu_view.resolution, 2u, 1u);
        if (render_view.get_shading_rate_di() == ShadingRate::QUARTER_RATE) shading_res = div_up(render_view.gpu_view.resolution, 2u, 2u);

        /* Direct illumination pass */
        render_graph.add_compute_pass("direct illumination pass", "lighting/direct_illumination.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(scene_view.scene_view) /* Scene view buffer */
            .read(render_view.blue_noise2d->image) /* Blue noise texture */
            .read(scene_view.bvh_nodes) /* TLAS nodes buffer */
            .read(scene_view.object_indices) /* Voxel object indices buffer */
            .read(scene_view.object_data) /* Voxel objects buffer */
            .read(scene_view.lights_data) /* Lights data buffer */
            // .write(render_view.macrofacet_cache) /* Cache buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .write(diffuse_image) /* Diffuse buffer */
            .group_size(16, 8)
            .work_size(shading_res.x, shading_res.y);
    }

    const glm::uvec2 quarter_rate = div_up(render_view.gpu_view.resolution, 2u, 2u);

    /* Reflections pass */
    render_graph.add_compute_pass("reflections pass", "lighting/reflections.cs")
        .read(render_view.render_view_buffer) /* Render view buffer */
        .read(scene_view.scene_view) /* Scene view buffer */
        .read(engine.renderer.linear_sampler)
        .read(render_view.blue_noise2d->image) /* Blue noise texture */
        .read(scene_view.bvh_nodes) /* TLAS nodes buffer */
        .read(scene_view.object_indices) /* Voxel object indices buffer */
        .read(scene_view.object_data) /* Voxel objects buffer */
        // .read(scene_view.lights_data) /* Lights data buffer */
        .write(render_view.macrofacet_cache) /* Cache buffer */
        .read(render_view.vbuffer.image) /* Visibility buffer */
        .write(raw_specular_image) /* Specular buffer */
        .group_size(16, 8)
        .work_size(quarter_rate.x, quarter_rate.y);
    
    for (uint32_t i = 0u; i < 6u; ++i) {
        /* Denoising pass */
        uint32_t step_size = 1u << i;
        render_graph.add_compute_pass("denoise pass", "lighting/wavelet_denoise.cs")
            .push_constants(&step_size, 0u, sizeof(uint32_t))
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(scene_view.object_data) /* Voxel objects buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .read(((i & 0b1u) == 0u) ? raw_specular_image : specular_image) /* Luminance buffer */
            .write(((i & 0b1u) == 0u) ? specular_image : raw_specular_image) /* Luminance buffer */
            .group_size(16, 8)
            .work_size(quarter_rate.x, quarter_rate.y);
    }

    // struct DenoiseOptions {
    //     float sample_radius = 8.0f;
    //     float color_weight = 1.0f;
    //     float normal_weight = 1.0f;
    //     float pos_weight = 1.0f;
    // } options;
    // options.sample_radius = 32.0f;
    // options.color_weight = 1.0f;
    // options.normal_weight = 1.0f;
    // options.pos_weight = 1.0f;

    // render_graph.add_compute_pass("denoise pass", "lighting/denoise.cs")
    //     .push_constants(&options, 0u, sizeof(DenoiseOptions))
    //     .read(render_view.render_view_buffer) /* Render view buffer */
    //     .read(render_view.blue_noise2d->image) /* Blue noise texture */
    //     .read(scene_view.object_data) /* Voxel objects buffer */
    //     .write(render_view.macrofacet_cache) /* Cache buffer */
    //     .read(render_view.vbuffer.image) /* Visibility buffer */
    //     .read(raw_specular_image) /* Luminance buffer */
    //     .write(specular_image) /* Luminance buffer */
    //     .group_size(16, 8)
    //     .work_size(quarter_rate.x, quarter_rate.y);

    // render_graph.add_compute_pass("blit pass", "blit.cs")
    //     .read(render_view.nbuffer.image) /* Luminance buffer */
    //     .write(diffuse_image) /* Luminance buffer */
    //     .group_size(16, 8)
    //     .work_size(render_res.x, render_res.y);

    /* Cache insert pass */
    render_graph.add_compute_pass("cache insert pass", "lighting/cache_insert.cs")
        .read(render_view.render_view_buffer) /* Render view buffer */
        .write(render_view.macrofacet_cache) /* Cache buffer */
        .read(render_view.vbuffer.image) /* Visibility buffer */
        .read(diffuse_image) /* Diffuse buffer */
        .read(specular_image) /* Specular buffer */
        .group_size(16, 8)
        .work_size(render_res.x, render_res.y);

    /* Cache flush pass */
    render_graph.add_compute_pass("cache flush pass", "lighting/cache_flush.cs")
        .read(render_view.render_view_buffer) /* Render view buffer */
        .write(render_view.macrofacet_cache) /* Cache buffer */
        .read(render_view.vbuffer.image) /* Visibility buffer */
        .group_size(16, 8)
        .work_size(render_res.x, render_res.y);
    
    // { /* Global illumination pass */
    //     glm::uvec2 shading_res = render_view.gpu_view.resolution;
    //     if (render_view.get_shading_rate_gi() == ShadingRate::HALF_RATE) shading_res = div_up(render_view.gpu_view.resolution, 2u, 1u);
    //     if (render_view.get_shading_rate_gi() == ShadingRate::QUARTER_RATE) shading_res = div_up(render_view.gpu_view.resolution, 2u, 2u);

    //     render_graph.add_compute_pass("global illumination pass", "global_illumination.cs")
    //         .read(render_view.render_view_buffer) /* Render view buffer */
    //         .read(scene_view.scene_view) /* Scene view buffer */
    //         .read(render_view.blue_noise->image) /* Blue noise texture */
    //         .read(engine.renderer.linear_sampler)
    //         .read(scene_view.bvh_nodes) /* TLAS nodes buffer */
    //         .read(scene_view.object_indices) /* Voxel object indices buffer */
    //         .read(scene_view.object_data) /* Voxel objects buffer */
    //         .write(render_view.macrofacet_cache) /* Cache buffer */
    //         .read(render_view.vbuffer.image) /* Visibility buffer */
    //         .write(diffuse_image) /* Luminance buffer */
    //         .group_size(16, 8)
    //         .work_size(shading_res.x, shading_res.y);
    // }
    
    /* Composite pass */
    if (engine.renderer.display_mode == DisplayMode::DEFAULT) {
        render_graph.add_compute_pass("composite pass", "composite.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(scene_view.scene_view) /* Scene view buffer */
            .read(engine.renderer.linear_sampler)
            .read(scene_view.object_data) /* Voxel objects buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .write(diffuse_image) /* Luminance Output */
            .group_size(16, 8)
            .work_size(render_res.x, render_res.y);
    }

    /* Debug visualizations */
    if (engine.renderer.display_mode == DisplayMode::ILLUMINANCE) {
        render_graph.add_compute_pass("[debug] illuminance pass", "debug/illuminance.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .read(diffuse_image) /* Luminance buffer */
            .write(render_image) /* Render target */
            .group_size(16, 8)
            .work_size(render_res.x, render_res.y);
    }
    if (engine.renderer.display_mode == DisplayMode::CACHE) {
        render_graph.add_compute_pass("[debug] cache pass", "debug/cache.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .write(render_image) /* Render target */
            .group_size(16, 8)
            .work_size(render_res.x, render_res.y);
    }
}

/* clang-format on */

void DiPipeline::deinit(GPUAdapter&) {}

}  // namespace tmt
