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

    VRAMBank& bank = gpu.get_vram_bank();

    /* Initialize the Render Target */
    const TargetDesc target {engine.window.get_window_handle()};
    if (const Result r = bank.create_render_target(target); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize render target.\nreason: {}", r.unwrap_err());
        return;
    } else {
        render_target = r.unwrap();
    }

    /* Create the active render view buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Constant | BufferUsage::TransferDst, sizeof(RenderView)); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to create render view buffer.\nreason: {}", r.unwrap_err().c_str());
        return;
    } else {
        render_view_buffer = r.unwrap();
    }

    /* Initialize pipelines */
    debug_pipeline.init(gpu);
    geometry_pipeline.init(gpu);
}

void Renderer::update() {
    // Temporary
    draw_line({-0.5f, 0.5f, 0.0f}, {0.0f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    draw_line({0.0f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    draw_line({0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});

    draw_line({0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    if (engine.window.resized) {
        if (const Result r = gpu.get_vram_bank().resize_render_target(render_target, engine.window.width, engine.window.height); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to resize the swapchain.\nreason: {}", r.unwrap_err().c_str());
        } else {
            engine.window.resized = false;
            Log::info(Log::Scope::RENDERER, "Swapchain has been resized.");
        }
    }

    Entity cam_entity = Camera::get_active_camera();
    if (cam_entity == entt::null) {
        Log::error(Log::Scope::RENDERER, "Camera Entity is NULL. Are there any active cameras in the scene?");
        return;
    }

    render_graph.new_graph().unwrap();

    /* Get active camera */
    Camera& camera = engine.ecs.get_component<Camera>(cam_entity);
    Transform& transform = engine.ecs.get_component<Transform>(cam_entity);

    render_view.resolution = glm::uvec2(engine.window.width, engine.window.height);
    const float aspect_ratio = (float)engine.window.width / (float)engine.window.height;

    /* Iterate over all cameras to find an active one to use as render view */
    static float rotation = 0.0f;
    rotation += 0.01f;
    glm::mat4 p = glm::perspective(glm::radians(camera.fov), aspect_ratio, 0.05f, 1000.0f);
    const glm::mat4 v = glm::inverse(transform.get_world_matrix());
    p[1][1] *= -1.0f;
    render_view.world_to_clip = p * v;
    render_view.clip_to_world = glm::inverse(render_view.world_to_clip);
    render_view.origin = glm::vec4(transform.get_world_position(), 0.0f);

    /* Upload the active render view */
    render_graph.upload_buffer(render_view_buffer, &render_view, 0u, sizeof(RenderView));

    /* Pipelines enqueue */
    geometry_pipeline.enqueue(render_graph, render_view_buffer, render_target);
    debug_pipeline.enqueue(render_graph, render_view_buffer, render_target);

    /* Add the immediate mode GUI to the render graph */
    if (imgui != nullptr) {
        render_graph.add_imgui(*imgui, render_target);
    }

    /* Compile the render graph */
    if (const Result r = render_graph.end_graph(); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to compile render graph.\nreason: {}", r.unwrap_err());
    }
    /* Dispatch the render graph */
    if (const Result r = render_graph.dispatch(); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to dispatch render graph.\nreason: {}", r.unwrap_err());
    }
}

void Renderer::end() {
    VRAMBank& bank = gpu.get_vram_bank();
    bank.destroy(render_view_buffer);
    bank.destroy(render_target);

    /* Pipelines cleanup */
    debug_pipeline.deinit(gpu);
    geometry_pipeline.deinit(gpu);

    /* Cleanup the VRAM bank & GPU adapter */
    render_graph.deinit().expect("failed to destroy render graph.");
    bank.deinit().expect("failed to destroy vram bank.");
    gpu.deinit().expect("failed to destroy gpu adapter.");
}

void Renderer::set_imgui(ImGUI* new_imgui, ImGUIFunctions functions) {
    imgui = new_imgui;
    /* Initialize the immediate mode GUI */
    if (const Result r = imgui->init(gpu, render_target, functions); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize imgui.\nreason: {}", r.unwrap_err());
        return;
    }
}

void Renderer::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color) { debug_pipeline.draw_line(start, end, color); }

}  // namespace tmt
