#include "renderer.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>

#include "core/window.hpp"
#include "core/logger.hpp"
#include "core/ecs.hpp"
#include "core/components/camera.hpp"
#include "core/components/transform.hpp"

#include "engine/engine.hpp"

#include "pipelines/debug_pipeline.hpp"
#include "pipelines/geometry_pipeline.hpp"
#include "tools/profiler.hpp"

namespace tmt {

Renderer::Renderer() : gpu(*new GPUAdapter()), render_graph(*new RenderGraph()), debug_pipeline(*new DebugPipeline()), geometry_pipeline(*new GeometryPipeline()) {}

Renderer::~Renderer() {
    delete &geometry_pipeline;
    delete &debug_pipeline;
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

    /* Initialize the GPU adapter */
    if (const Result r = gpu.init(true); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize gpu adapter.\nreason: {}", r.unwrap_err());
        return;
    }

    /* Initialize the Render Graph */
    render_graph.set_shader_path("assets/engine/shaders/bin");
    render_graph.set_staging_limit(10000000u /* 10mb */);
    render_graph.set_max_graphs_in_flight(2u); /* Double buffering */
    if (const Result r = render_graph.init(gpu); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize render graph.\nreason: {}", r.unwrap_err());
        return;
    }

    /* Initialize the Render View */
    render_view.init();

    /* Initialize pipelines */
    debug_pipeline.init(gpu);
    geometry_pipeline.init(gpu);
}

void Renderer::update() {
    TMT_ZONE_SCOPED_N("Rendering")
    // Temporary
    draw_line({-0.5f, 0.5f, 0.0f}, {0.0f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    draw_line({0.0f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    draw_line({0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});

    draw_line({0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    render_view.update();

    Entity cam_entity = Camera::get_active_camera();
    if (cam_entity == entt::null) {
        Log::error(Log::Scope::RENDERER, "Camera Entity is NULL. Are there any active cameras in the scene?");
    }

    render_graph.new_graph().unwrap();

    if (cam_entity != entt::null) {
        render_view.update_gpu_view(render_graph, cam_entity);

        /* Pipelines enqueue */
        geometry_pipeline.enqueue(render_graph, render_view);
        debug_pipeline.enqueue(render_graph, render_view);
    }

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
    render_view.deinit();

    /* Pipelines cleanup */
    debug_pipeline.deinit(gpu);
    geometry_pipeline.deinit(gpu);

    /* Cleanup the VRAM bank & GPU adapter */
    render_graph.deinit().expect("failed to destroy render graph.");
    bank.deinit().expect("failed to destroy vram bank.");
    gpu.deinit().expect("failed to destroy gpu adapter.");
}

#ifdef THERMITE_EDITOR
void Renderer::set_imgui(ImGUI* new_imgui) {
    imgui = new_imgui;
    /* Initialize the immediate mode GUI */
    if (const Result r = imgui->init(gpu, render_view.render_target); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize imgui.\nreason: {}", r.unwrap_err());
        return;
    }

    render_view.imgui_viewport = imgui->add_image(render_view.viewport_image);
}
#endif

void Renderer::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color) { debug_pipeline.draw_line(start, end, color); }

VRAMBank& Renderer::vram_bank() { return gpu.get_vram_bank(); }

}  // namespace tmt
