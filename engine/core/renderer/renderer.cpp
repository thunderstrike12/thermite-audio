#include "renderer.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

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

    /* Initialize the immediate mode GUI */
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(engine.window.window);
    if (const Result r = imgui.init(gpu, render_target, IMGUI_FUNCTIONS); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize imgui.\nreason: {}", r.unwrap_err());
        return;
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

    /* Start a new imgui frame */
    imgui.new_frame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    render_graph.new_graph().unwrap();

    /* Capture all cameras in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const Camera>(entt::get<Transform>);
    render_view.resolution = glm::uvec2(engine.window.width, engine.window.height);
    const float aspect_ratio = (float)engine.window.width / (float)engine.window.height;

    /* Iterate over all cameras to find an active one to use as render view */
    static float rotation = 0.0f;
    rotation += 0.01f;
    for (auto&& [entity, camera, transform] : group.each()) {
        if (camera.active == true) {
            const glm::mat4 p = glm::perspectiveLH(glm::radians(camera.fov), aspect_ratio, 0.05f, 1000.0f);
            const glm::mat4 v = glm::inverse(transform.get_world_matrix());
            render_view.world_to_clip = p * v;
            render_view.clip_to_world = glm::inverse(render_view.world_to_clip);
            render_view.origin = glm::vec4(transform.get_world_position(), 0.0f);
            ImGui::Begin("Camera");
            glm::vec3 pos = transform.get_world_position();
            if (ImGui::InputFloat3("Origin", &pos.x)) {
                transform.set_world_position(pos);
            }
            ImGui::End();
            break;
        }
    }

    /* Upload the active render view */
    render_graph.upload_buffer(render_view_buffer, &render_view, 0u, sizeof(RenderView));

    /* Pipelines enqueue */
    geometry_pipeline.enqueue(render_graph, render_view_buffer, render_target);
    debug_pipeline.enqueue(render_graph, render_target);

    /* Add the immediate mode GUI to the render graph */
    render_graph.add_imgui(imgui, render_target);

    /* End the imgui frame */
    ImGui::Render();

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

void Renderer::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color) { debug_pipeline.draw_line(start, end, color); }

}  // namespace tmt
