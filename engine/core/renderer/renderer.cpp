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

    VRAMBank& bank = gpu.get_vram_bank();

    /* Initialize the Render Target */
    const TargetDesc target {engine.window.get_window_handle()};
    if (const Result r = bank.create_render_target(target); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize render target.\nreason: {}", r.unwrap_err());
        return;
    } else {
        render_target = r.unwrap();
    }

    /* Viewport Texture */
    if (const Result r = bank.create_texture(
            TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::RGBA8Unorm, {(uint32_t)engine.window.width, (uint32_t)engine.window.height, 0}
        );
        r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize attachment texture.\nreason: {}", r.unwrap_err());
        return;
    } else
        viewport_texture = r.unwrap();
    /* Viewport Image */
    if (const Result r = bank.create_image(viewport_texture); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize attachment image.\nreason: {}", r.unwrap_err());
        return;
    } else
        viewport_image = r.unwrap();

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
    TMT_ZONE_SCOPED_N("Rendering")
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
    }

    render_graph.new_graph().unwrap();

#ifndef THERMITE_EDITOR
    render_view.resolution = glm::uvec2(engine.window.width, engine.window.height);
#endif  // !THERMITE_EDITOR
    if (cam_entity != entt::null) {
        const float aspect_ratio = (float)render_view.resolution.x / (float)render_view.resolution.y;
        /* Get active camera */
        Camera& camera = engine.ecs.get_component<Camera>(cam_entity);
        Transform& transform = engine.ecs.get_component<Transform>(cam_entity);

        /* Iterate over all cameras to find an active one to use as render view */
        glm::mat4 p = glm::perspective(glm::radians(camera.fov), aspect_ratio, 0.05f, 1000.0f);
        const glm::mat4 v = glm::inverse(transform.get_world_matrix());
        p[1][1] *= -1.0f;
        render_view.world_to_clip = p * v;
        render_view.clip_to_world = glm::inverse(render_view.world_to_clip);
        render_view.origin = glm::vec4(transform.get_world_position(), 0.0f);

        /* Upload the active render view */
        render_graph.upload_buffer(render_view_buffer, &render_view, 0u, sizeof(RenderView));
        /* Pipelines enqueue */
        geometry_pipeline.enqueue(render_graph, render_view_buffer);
        debug_pipeline.enqueue(render_graph, render_view_buffer);
    }

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
    TMT_FRAME_MARK;
}

void Renderer::end() {
    VRAMBank& bank = gpu.get_vram_bank();
    bank.destroy(render_view_buffer);
    bank.destroy(viewport_texture);
    bank.destroy(viewport_image);
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

    imgui_viewport = imgui->add_image(viewport_image);
}

BindHandle Renderer::get_render_image() const {
#ifdef THERMITE_EDITOR
    return viewport_image;
#else
    return render_target;
#endif  // THERMITE_EDITOR
}

u64 Renderer::get_imgui_viewport() const { return imgui_viewport; }

void Renderer::set_viewport_size(uint32_t width, uint32_t height) {
    if (width != render_view.resolution.x || height != render_view.resolution.y) {
        render_view.resolution = {width, height};

        if (const Result r = gpu.get_vram_bank().resize_texture(viewport_texture, {width, height, 0}); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to resize the viewport texture.\nreason: {}", r.unwrap_err().c_str());
        } else {
            Log::info(Log::Scope::RENDERER, "viewport texture has been resized.");
        }

        imgui->remove_image(viewport_image);
        imgui_viewport = imgui->add_image(viewport_image);
    }
}

void Renderer::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color) { debug_pipeline.draw_line(start, end, color); }

}  // namespace tmt
