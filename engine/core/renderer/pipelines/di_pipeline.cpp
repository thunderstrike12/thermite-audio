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
#include "engine/tools/player_data.hpp"

#include "engine/core/input/input.hpp"

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
    if (engine.renderer.display_mode != DisplayMode::ILLUMINANCE && engine.renderer.display_mode != DisplayMode::CACHE && engine.renderer.display_mode != DisplayMode::LIGHTS && engine.renderer.display_mode != DisplayMode::DEFAULT ) return;

    /* Gather buffer handles */
    const BindHandle render_image = render_view.get_render_image();
    const BindHandle diff_buffer = render_view.diff_buffer.image, raw_diff_buffer = render_view.lbuffer.image;
    const BindHandle spec_buffer = render_view.spec_buffer.image, raw_spec_buffer = render_view.lbuffer.image;
    const BindHandle output_buffer = render_view.lbuffer.image;
    const Size3D output_res { render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y };
    const RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");

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

    { /* Diffuse illumination pass */
        const Size3D rate = rated_resolution(output_res, settings.diff_shading_rate);
        render_graph.add_compute_pass("diffuse pass", "lighting/diffuse.cs")
            /* Render & Scene view */
            .read(render_view.render_view_buffer)
            .read(scene_view.scene_view)
            /* Noise texture */
            .read(render_view.blue_noise2d->image)
            /* Ray-tracing buffers */
            .read(scene_view.bvh_nodes)
            .read(scene_view.object_indices)
            .read(scene_view.object_data)
            /* Light data */
            .read(scene_view.lights_data)
            /* Lights grid */
            .read(scene_view.light_grid) 
            /* Visibility buffer & Output buffer */
            .read(render_view.vbuffer.image)
            .write(diff_buffer)
            .push_constants(&scene_view.light_grid_center, 0, sizeof(glm::vec3))
            .group_size(16, 8)
            .work_size(rate.x, rate.y);

        render_graph.add_compute_pass("ambient pass", "lighting/ambient.cs")
            /* Render & Scene view */
            .read(render_view.render_view_buffer)
            .read(scene_view.scene_view)
            /* Noise texture */
            .read(render_view.blue_noise2d->image)
            /* Ray-tracing buffers */
            .read(scene_view.bvh_nodes)
            .read(scene_view.object_indices)
            .read(scene_view.object_data)
            /* Visibility buffer & Output buffer */
            .read(render_view.vbuffer.image)
            .write(diff_buffer)
            .read(engine.renderer.linear_sampler)
            .group_size(16, 8)
            .work_size(rate.x, rate.y);
    }

    { /* Specular illumination pass */
        const Size3D rate = rated_resolution(output_res, settings.spec_shading_rate);
        render_graph.add_compute_pass("specular pass", "lighting/specular.cs")
            /* Render & Scene view */
            .read(render_view.render_view_buffer)
            .read(scene_view.scene_view)
            /* Sampler & Noise texture */
            .read(engine.renderer.linear_sampler)
            .read(render_view.blue_noise2d->image)
            /* Ray-tracing buffers */
            .read(scene_view.bvh_nodes)
            .read(scene_view.object_indices)
            .read(scene_view.object_data)
            /* Lighting cache (for reflecting environment) */
            .write(render_view.macrofacet_cache)
            /* Visibility buffer & Output buffer */
            .read(render_view.vbuffer.image)
            .write(raw_spec_buffer)
            .group_size(16, 8)
            .work_size(rate.x, rate.y);
        
        /* Specular denoising */
        for (uint32_t i = 0u; i < 6u; ++i) {
            uint32_t step_size = 1u << i;
            render_graph.add_compute_pass("specular denoise pass", "lighting/specular_denoise.cs")
                /* Render view */
                .read(render_view.render_view_buffer) 
                /* Voxel objects */
                .read(scene_view.object_data)
                /* Visibility buffer */
                .read(render_view.vbuffer.image) 
                /* Luminance input & output buffers */
                .read(((i & 0b1u) == 0u) ? raw_spec_buffer : spec_buffer)
                .write(((i & 0b1u) == 0u) ? spec_buffer : raw_spec_buffer)
                /* Step size push constant */
                .push_constants(&step_size, 0u, sizeof(uint32_t))
                .group_size(16, 8)
                .work_size(rate.x, rate.y);
        }
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
        .read(diff_buffer) /* Diffuse buffer */
        .read(spec_buffer) /* Specular buffer */
        .group_size(16, 8)
        .work_size(output_res.x, output_res.y);

    /* Cache flush pass */
    render_graph.add_compute_pass("cache flush pass", "lighting/cache_flush.cs")
        .read(render_view.render_view_buffer) /* Render view buffer */
        .write(render_view.macrofacet_cache) /* Cache buffer */
        .read(scene_view.object_data) /* Voxel objects buffer */
        .read(render_view.vbuffer.image) /* Visibility buffer */
        .group_size(16, 8)
        .work_size(output_res.x, output_res.y);
    
    /* Composite pass */
    if (engine.renderer.display_mode == DisplayMode::DEFAULT || engine.renderer.display_mode == DisplayMode::LIGHTS) {
        render_graph.add_compute_pass("composite pass", "composite.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(scene_view.scene_view) /* Scene view buffer */
            .read(engine.renderer.linear_sampler)
            .read(scene_view.object_data) /* Voxel objects buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .write(output_buffer) /* Luminance Output */
            .group_size(16, 8)
            .work_size(output_res.x, output_res.y);
    }

    /* Debug visualizations */
    if (engine.renderer.display_mode == DisplayMode::ILLUMINANCE) {
        render_graph.add_compute_pass("[debug] illuminance pass", "debug/illuminance.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            //.read(render_view.vbuffer.image) /* Visibility buffer */
            //.write(render_view.macrofacet_cache) /* Cache buffer */
            .read(diff_buffer) /* Luminance buffer */
            .write(render_image) /* Render target */
            .group_size(16, 8)
            .work_size(output_res.x, output_res.y);
    }
    if (engine.renderer.display_mode == DisplayMode::CACHE) {
        render_graph.add_compute_pass("[debug] cache pass", "debug/cache.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .write(render_view.macrofacet_cache) /* Cache buffer */
            .write(render_image) /* Render target */
            .group_size(16, 8)
            .work_size(output_res.x, output_res.y);
    }
}

/* clang-format on */

void DiPipeline::deinit(GPUAdapter&) {}

}  // namespace tmt
