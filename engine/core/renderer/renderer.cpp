#include "renderer.hpp"

#include <graphite/imgui.hh>
#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>
#include <graphite/nodes/compute_node.hh>

#include "core/io.hpp"
#include "core/ecs.hpp"
#include "core/window.hpp"
#include "core/logger.hpp"
#include "core/components/camera.hpp"
#include "core/components/transform.hpp"

#include "engine/engine.hpp"

#include "pipelines/lighting_pipeline.hpp"
#include "pipelines/ui_pipeline.hpp"
#include "pipelines/vfx_pipeline.hpp"
#include "pipelines/polyline_pipeline.hpp"
#include "pipelines/geometry_pipeline.hpp"
#include "pipelines/post_process_pipeline.hpp"

#include "tools/profiler.hpp"
#include "tools/player_data.hpp"

namespace tmt {

Renderer::Renderer() :
    gpu(*new GPUAdapter()),
    render_graph(*new RenderGraph()),
    geometry_pipeline(*new GeometryPipeline()),
    lighting_pipeline(*new LightingPipeline()),
    polyline_pipeline(*new PolylinePipeline()),
    vfx_pipeline(*new VfxPipeline()),
    ui_pipeline(*new UiPipeline()),
    post_process_pipeline(*new PostProcessPipeline()) {}

Renderer::~Renderer() {
    delete &polyline_pipeline;
    delete &lighting_pipeline;
    delete &vfx_pipeline;
    delete &geometry_pipeline;
    delete &ui_pipeline;
    delete &post_process_pipeline;

    delete &render_graph;
    delete &gpu;
}

/* Custom thermite logger function for graphite. */
void thermite_logger(const DebugSeverity severity, const char* msg, void*) {
    switch (severity) {
        case DebugSeverity::Info:
            Log::info(Log::Scope::RENDERER, "{}", msg);
            break;
        case DebugSeverity::Warning:
            Log::warn(Log::Scope::RENDERER, "{}", msg);
            break;
        case DebugSeverity::Error:
            Log::error(Log::Scope::RENDERER, "{}", msg);
            break;
    }
}

void Renderer::init() {
    gpu.set_logger(thermite_logger, DebugLevel::Verbose);
    gpu.set_max_render_targets(4u);
    gpu.set_max_buffers(8192u);
    gpu.set_max_textures(256u);
    gpu.set_max_images(256u);
    gpu.set_max_samplers(32u);

    /* Initialize the GPU adapter */
    bool sync_validation = false;  // Sync validation is enabled only in Debug
    bool gpu_validation = false;   // GPU assisted validation is enabled only in Debug
#ifdef THERMITE_DEBUG
    sync_validation = true;
    gpu_validation = true;
#endif  //

    if (const Result r = gpu.init(true, sync_validation, gpu_validation); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize gpu adapter.\nreason: {}", r.unwrap_err());
        return;
    }

    /* Initialize the Render Graph */
    IO::FileLocation shader_location { IO::Location::ENGINE, "shaders/bin" };
    render_graph.set_shader_path(shader_location.get_relative_path().string().c_str());
    render_graph.set_staging_limit(32000000u /* 32mb */);
    render_graph.set_max_graphs_in_flight(2u); /* Double buffering */
    if (const Result r = render_graph.init(gpu); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize render graph.\nreason: {}", r.unwrap_err());
        return;
    }

    /* Initialize the rendering views */
    render_view.init();
    scene_view.init();

    /* Initialize pipelines */
    polyline_pipeline.init(gpu);
    lighting_pipeline.init(gpu);
    vfx_pipeline.init(gpu);
    ui_pipeline.init(gpu);
    post_process_pipeline.init(gpu);

    VRAMBank& bank = gpu.get_vram_bank();

    /* Initialize the Samplers */
    linear_sampler = bank.create_sampler("Linear Sampler").expect("failed to initialize linear sampler.");
    point_sampler = bank.create_sampler("Point Sampler", Filter::Nearest).expect("failed to initialize linear sampler.");
    down_sampler = bank.create_sampler("Down Sampler", Filter::Linear, AddressMode::ClampToBorder).expect("failed to create down sampler.");
    up_sampler = bank.create_sampler("Up Sampler", Filter::Linear, AddressMode::ClampToEdge).expect("failed to create up sampler.");

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

    debug_transform.set_local_position({ 0.0f, 0.0f, -1.0f });
}

void Renderer::update() {
    TMT_ZONE_SCOPED_N("Rendering")

    /* Start a new render graph */
    render_graph.new_graph().unwrap();

    /* Update the rendering views */
    render_view.update();
    scene_view.update(render_graph, render_view);

    /* Update the camera view */
    Entity cam_entity = Camera::get_active_camera();
    if (engine.game_controller.is_running() && cam_entity != entt::null) {
        const Camera& camera = engine.ecs.get_component<Camera>(cam_entity);
        const Transform& transform = engine.ecs.get_component<Transform>(cam_entity);
        render_view.update_gpu_view(render_graph, camera, transform);
    } else {
        render_view.update_gpu_view(render_graph, debug_camera, debug_transform);
    }

    /* Geometry & Lighting */
    geometry_pipeline.enqueue(render_graph, render_view, scene_view);
    lighting_pipeline.enqueue(render_graph, render_view, scene_view);

    RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");
    const uint32_t frame_flag = (render_view.frame_counter & 1) == 0;
    { /* Auto exposure */
        /* clang-format off */
        /* Initial 256x256 aliased gather */
        render_graph.add_compute_pass("autox gather", "lighting/autox_gather.cs")
            .read(render_view.render_view_buffer)
            .read(render_view.lbuffer.image)
            .write(autox_partial_image)
            .group_size(8, 8)
            .work_size(256, 256);

        /* 256x256 to 16x16 downsampling using LDS */
        render_graph.add_compute_pass("autox average", "lighting/autox_average.cs")
            .read(autox_partial_image)
            .write(autox_tiny_image)
            .read(down_sampler)
            .group_size(8, 8)
            .work_size(128, 128);

        /* Final 16x16 average and temporal response */
        render_graph.add_compute_pass("autox final", "lighting/autox_final.cs")
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
        render_graph.add_compute_pass("autox apply", "lighting/autox_apply.cs")
            .write(render_view.lbuffer.image)
            .read(autox_image)
            .push_constants(&autox_constants, 0u, sizeof(AutoXConstants))
            .group_size(16, 8)
            .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
        /* clang-format on */
    }

    /* Sky composite pass */
    if (engine.renderer.display_mode == DisplayMode::DEFAULT) {
        /* clang-format off */
        render_graph.add_compute_pass("sky composite pass", "sky.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(scene_view.scene_view) /* Scene view buffer */
            .read(engine.renderer.linear_sampler)
            .read(frame_flag ? render_view.depth_image : render_view.prev_depth_image) /* Depth buffer */
            .write(render_view.lbuffer.image) /* Luminance Output */
            .group_size(16, 8)
            .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
        /* clang-format on */
    }

    /* VFX */
    vfx_pipeline.enqueue(render_graph, render_view);

    if (engine.renderer.display_mode == DisplayMode::MOTIONVECTORS) {
        render_graph.add_compute_pass("[debug] motion vectors pass", "debug/motion_vectors.cs")
            .read(render_view.mbuffer.image)
            .write(render_view.get_render_image())
            .group_size(16, 8)
            .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
    }

    /* Post Processing */
    if (engine.renderer.display_mode == DisplayMode::DEFAULT) {
        post_process_pipeline.enqueue(render_graph, render_view);
    }

    if (scene_view.render_outlines) {
        /* Object outline render pass */
        render_graph.add_compute_pass("object outline", "outline.cs")
            .read(render_view.render_view_buffer)
            .read(scene_view.object_data)
            .read(render_view.vbuffer.image)
            .write(render_view.get_render_image())
            .group_size(16, 8)
            .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
    }

    if (engine.renderer.display_mode == DisplayMode::LIGHTS) {
        render_graph.add_compute_pass("[debug] lights pass", "debug/lights.cs")
            .read(render_view.render_view_buffer)
            .read(scene_view.object_data)
            .read(render_view.vbuffer.image)
            .read(scene_view.light_grid)
            .write(render_view.get_render_image())
            .group_size(16, 8)
            .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
    }

    /* Polyline & UI */
    polyline_pipeline.enqueue(render_graph, render_view);
    ui_pipeline.enqueue(render_graph, render_view);

#ifdef THERMITE_EDITOR
    /* Add the immediate mode GUI to the render graph */
    if (imgui != nullptr) {
        render_graph.add_imgui(*imgui, render_view.render_target);
    }
#endif

    /* Compile the render graph */
    if (const Result r = render_graph.end_graph(); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to compile render graph.\nreason: {}", r.unwrap_err());
    }
    /* Dispatch the render graph */
    if (const Result r = render_graph.dispatch(); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to dispatch render graph.\nreason: {}", r.unwrap_err());
    }
    TMT_FRAME_MARK;
}

void Renderer::end() {
    VRAMBank& bank = gpu.get_vram_bank();

    /* De-initialize the rendering views */
    render_view.deinit();
    scene_view.deinit();

    /* Pipelines cleanup */
    polyline_pipeline.deinit(gpu);
    lighting_pipeline.deinit(gpu);
    vfx_pipeline.deinit(gpu);
    ui_pipeline.deinit(gpu);
    post_process_pipeline.deinit(gpu);

    bank.destroy(linear_sampler);
    bank.destroy(point_sampler);
    bank.destroy(down_sampler);
    bank.destroy(up_sampler);

    bank.destroy(autox_partial_image);
    bank.destroy(autox_partial_texture);
    bank.destroy(autox_tiny_image);
    bank.destroy(autox_tiny_texture);
    bank.destroy(autox_image);
    bank.destroy(autox_texture);

    /* Cleanup the VRAM bank & GPU adapter */
    render_graph.deinit().expect("failed to destroy render graph.");
    bank.deinit().expect("failed to destroy vram bank.");
    gpu.deinit().expect("failed to destroy gpu adapter.");
}

Hit Renderer::trace_ray(const Ray& ray) const {
    Hit hit = scene_view.bvh.trace(ray);
    if (hit) hit.entity = scene_view.entities[(uint32_t)hit.entity];
    return hit;
}

#ifdef THERMITE_EDITOR
void Renderer::set_imgui(ImGUI* new_imgui) {
    imgui = new_imgui;
    /* Initialize the immediate mode GUI */
    if (const Result r = imgui->init(gpu, render_view.render_target); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize imgui.\nreason: {}", r.unwrap_err());
        return;
    }

    render_view.imgui_viewport = imgui->add_image(render_view.viewport.image);
}
#endif

VRAMBank& Renderer::vram_bank() {
    return gpu.get_vram_bank();
}

void Renderer::destroy(OpaqueHandle& handle) {
    render_graph.defer_destroy(handle);
}

}  // namespace tmt
