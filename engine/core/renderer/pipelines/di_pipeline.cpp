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
    const glm::uvec2 render_res = render_view.gpu_view.resolution;

    if (cache_init == false) {
        /* Cache init pass */
        render_graph.add_compute_pass("cache init pass", "cache_init.cs")
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .group_size(128)
            .work_size(CACHE_SIZE);

        cache_init = true;
    } else {
        /* Cache eviction pass (amortize over 8 frames) */
        render_graph.add_compute_pass("cache eviction pass", "cache_evict.cs")
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
        render_graph.add_compute_pass("direct illumination pass", "direct_illumination.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(scene_view.scene_view) /* Scene view buffer */
            .read(render_view.blue_noise2d->image) /* Blue noise texture */
            .read(scene_view.bvh_nodes) /* TLAS nodes buffer */
            .read(scene_view.object_indices) /* Voxel object indices buffer */
            .read(scene_view.object_data) /* Voxel objects buffer */
            .read(scene_view.lights_data) /* Lights data buffer */
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .write(render_view.lbuffer.image) /* Luminance buffer */
            .group_size(16, 8)
            .work_size(shading_res.x, shading_res.y);
    }

    /* Cache flush pass */
    render_graph.add_compute_pass("cache flush pass", "cache_flush.cs")
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
    //         .write(render_view.lbuffer.image) /* Luminance buffer */
    //         .group_size(16, 8)
    //         .work_size(shading_res.x, shading_res.y);
    // }
        
    // for (uint32_t i = 0u; i < 6u; ++i) {
    //     /* Denoising pass */
    //     uint32_t step_size = 1u << i;
    //     render_graph.add_compute_pass("denoise pass", "wavelet_denoise.cs")
    //         .push_constants(&step_size, 0u, sizeof(uint32_t))
    //         .read(render_view.render_view_buffer) /* Render view buffer */
    //         .read(scene_view.object_data) /* Voxel objects buffer */
    //         .read(render_view.vbuffer.image) /* Visibility buffer */
    //         .read(((i & 0b1u) == 0u) ? render_view.lbuffer.image : render_view.nbuffer.image) /* Luminance buffer */
    //         .write(((i & 0b1u) == 0u) ? render_view.nbuffer.image : render_view.lbuffer.image) /* Luminance buffer */
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
            .write(render_view.lbuffer.image) /* Luminance buffer */
            .group_size(16, 8)
            .work_size(render_res.x, render_res.y);
    }

    /* Debug visualizations */
    if (engine.renderer.display_mode == DisplayMode::ILLUMINANCE) {
        render_graph.add_compute_pass("[debug] illuminance pass", "debug/illuminance.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .read(render_view.lbuffer.image) /* Luminance buffer */
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
