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

#include "pipelines/di_pipeline.hpp"
#include "pipelines/ui_pipeline.hpp"
#include "pipelines/vfx_pipeline.hpp"
#include "pipelines/polyline_pipeline.hpp"
#include "pipelines/geometry_pipeline.hpp"
#include "pipelines/post_process_pipeline.hpp"

#include "tools/profiler.hpp"

namespace tmt {

Renderer::Renderer() :
    gpu(*new GPUAdapter()),
    render_graph(*new RenderGraph()),
    geometry_pipeline(*new GeometryPipeline()),
    di_pipeline(*new DiPipeline()),
    polyline_pipeline(*new PolylinePipeline()),
    vfx_pipeline(*new VfxPipeline()),
    ui_pipeline(*new UiPipeline()),
    post_process_pipeline(*new PostProcessPipeline()) {}

Renderer::~Renderer() {
    delete &polyline_pipeline;
    delete &di_pipeline;
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
    di_pipeline.init(gpu);
    vfx_pipeline.init(gpu);
    ui_pipeline.init(gpu);
    post_process_pipeline.init(gpu);

    /* Initialize the Samplers */
    linear_sampler = gpu.get_vram_bank().create_sampler("Linear Sampler").expect("failed to initialize linear sampler.");
    point_sampler = gpu.get_vram_bank().create_sampler("Point Sampler", Filter::Nearest).expect("failed to initialize linear sampler.");

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

    /* Enqueue pipelines */
    geometry_pipeline.enqueue(render_graph, render_view, scene_view);
    di_pipeline.enqueue(render_graph, render_view, scene_view);
    vfx_pipeline.enqueue(render_graph, render_view);

    if (engine.renderer.display_mode == DisplayMode::MOTIONVECTORS) {
        render_graph.add_compute_pass("[debug] motion vectors pass", "debug/motion_vectors.cs")
            .read(render_view.mbuffer.image)
            .write(render_view.get_render_image())
            .group_size(16, 8)
            .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
    }

    /* TAA Resolve */
    if (engine.renderer.display_mode == DisplayMode::DEFAULT) {
        const uint32_t frame_flag = (render_view.frame_counter & 1) == 0;
        uint32_t taa_flag = engine.renderer.enable_taa ? 1u : 0u;
        /* clang-format off */
        render_graph.add_compute_pass("TAA Resolve", "taa_resolve.cs")
            .read(render_view.render_view_buffer)
            .read(point_sampler)
            .read(linear_sampler)
            .read(render_view.mbuffer.image)
            .write(render_view.lbuffer.image)
            .write(frame_flag ? render_view.hbuffer1.image : render_view.hbuffer2.image)
            .read(frame_flag ? render_view.hbuffer2.image : render_view.hbuffer1.image)
            .push_constants(&taa_flag, 0, sizeof(uint32_t))
            .group_size(16, 8)
            .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
    }

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
        /* clang-format on */
    }

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
    di_pipeline.deinit(gpu);
    vfx_pipeline.deinit(gpu);
    ui_pipeline.deinit(gpu);
    post_process_pipeline.deinit(gpu);

    bank.destroy(linear_sampler);
    bank.destroy(point_sampler);

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
