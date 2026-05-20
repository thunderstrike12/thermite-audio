#include "lighting_pipeline.hpp"

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

void LightingPipeline::init(GPUAdapter&) {}

/* clang-format off */

void LightingPipeline::enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view) {
    /* Only run this pass if it's outputs are actually used */
    if (engine.renderer.display_mode != DisplayMode::ILLUMINANCE && engine.renderer.display_mode != DisplayMode::CACHE && engine.renderer.display_mode != DisplayMode::LIGHTS && engine.renderer.display_mode != DisplayMode::DEFAULT ) return;

    /* Gather buffer handles */
    const BindHandle render_image = render_view.get_render_image();
    const BindHandle diff_buffer = render_view.diff_buffer.image, raw_diff_buffer = render_view.lbuffer.image;
    const BindHandle spec_buffer = render_view.spec_buffer.image, raw_spec_buffer = render_view.lbuffer.image;
    const BindHandle output_buffer = render_view.lbuffer.image;
    const Size3D output_res { render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y };
    const RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");

    { /* Light grid culling passes */
        render_graph.add_compute_pass("light grid clear counts pass", "lightgrid/clear_counts.cs")
            .write(scene_view.light_grid)
            .group_size(64)
            .work_size(light_grid::CASCADES_RESOLUTION * light_grid::CASCADES_RESOLUTION * light_grid::CASCADES_RESOLUTION * light_grid::MAX_CASCADES);

        render_graph.add_compute_pass("light culling pass", "lightgrid/cull.cs")
            .read(scene_view.cascades_bitmasks)
            .read(scene_view.lights_data)
            .write(scene_view.light_grid)
            .push_constants(&scene_view.light_grid_center, 0, sizeof(glm::vec3))
            .group_size(4, 4, 4)
            .work_size(light_grid::CASCADES_RESOLUTION, light_grid::CASCADES_RESOLUTION, light_grid::CASCADES_RESOLUTION * light_grid::MAX_CASCADES);
    }

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

    { /* Diffuse illumination passes */
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

    { /* Specular illumination passes */
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

    /* Froxel light volume passes */
    if (engine.renderer.display_mode == DisplayMode::DEFAULT) {
        /* Froxel volume update settings */
        struct Fpc {
            float fog_anisotropy = 0.6f;
            float scatter_strength = 0.0025f;
            float particle_reflectance = 0.01f;
        } froxel_p {};
        froxel_p.fog_anisotropy = scene_view.fog_anisotropy;
        froxel_p.scatter_strength = scene_view.fog_scatter_strength / 100.0f;
        froxel_p.particle_reflectance = scene_view.particle_reflectance;

        /* Froxel volume update pass */
        const bool flip = (render_view.frame_counter & 0b1u) == 0u;
        render_graph.add_compute_pass("froxel update pass", "lighting/froxel_trace.cs")
            /* Render & Scene view */
            .read(render_view.render_view_buffer)
            .read(scene_view.scene_view)
            /* Ray-tracing buffers */
            .read(scene_view.bvh_nodes)
            .read(scene_view.object_indices)
            .read(scene_view.object_data)
            /* Froxel buffers */
            .read(flip ? render_view.prev_froxel_scatter_image : render_view.froxel_scatter_image)
            .write(flip ? render_view.froxel_scatter_image : render_view.prev_froxel_scatter_image)
            .read(flip ? render_view.prev_froxel_luminance_image : render_view.froxel_luminance_image)
            .write(flip ? render_view.froxel_luminance_image : render_view.prev_froxel_luminance_image)
            .read(render_view.froxel_sampler)
            .push_constants(&froxel_p, 0u, sizeof(Fpc))
            .group_size(16, 8, 1)
            .work_size(160, 90, 64);

        /* Froxel volumetric integration settings */
        struct Vpc {
            glm::vec3 fog_absorb = glm::vec3(0.02f);
            float base_step_size = 0.1f;
            uint32_t step_count = 16u;
        } volume_p {};
        volume_p.fog_absorb = scene_view.fog_absorption;
        volume_p.base_step_size = settings.fog_base_step_size;
        volume_p.step_count = settings.fog_step_count;

        /* Froxel volumetric integration pass */
        render_graph.add_compute_pass("volumetric pass", "lighting/volumetric.cs")
            /* Render view */
            .read(render_view.render_view_buffer)
            .read(render_view.blue_noise1d->image)
            /* Vis buffer */
            .read(render_view.vbuffer.image)
            .read(engine.renderer.scene_view.object_data)
            /* Froxel data */
            .read(flip ? render_view.froxel_scatter_image : render_view.prev_froxel_scatter_image)
            .read(render_view.froxel_sampler)
            /* Output buffer */
            .write(output_buffer) 
            .push_constants(&volume_p, 0u, sizeof(Vpc))
            .group_size(16, 8)
            .work_size(output_res.x, output_res.y);
    }

    /* Debug visualizations */
    if (engine.renderer.display_mode == DisplayMode::ILLUMINANCE) {
        render_graph.add_compute_pass("[debug] illuminance pass", "debug/illuminance.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
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

void LightingPipeline::deinit(GPUAdapter&) {}

}  // namespace tmt
