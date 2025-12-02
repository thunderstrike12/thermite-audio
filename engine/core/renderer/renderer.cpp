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

#include "engine/engine.hpp"

#include "pipelines/debug_pipeline.hpp"

namespace tmt {

Renderer::Renderer() : gpu(*new GPUAdapter()), render_graph(*new RenderGraph()), debug_pipeline(*new DebugPipeline()) {}

Renderer::~Renderer() {
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

    /* Initialize Pipelines */
    debug_pipeline.init(gpu);
}

static bool is_init = false;
static Buffer vertex_buffer {};

struct Vertex {
    float x;
    float y;
    float z;
};

void Renderer::update() {
    VRAMBank& bank = gpu.get_vram_bank();
    if (!is_init) {
        /* Initialise a vertex buffer */

        if (const Result r = bank.create_buffer(BufferUsage::Vertex | BufferUsage::TransferDst, 1, sizeof(Vertex) * 3); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to initialise constant buffer.\nreason: {}", r.unwrap_err());
            return;
        } else {
            vertex_buffer = r.unwrap();
        }
        std::vector<Vertex> vertices {};
        vertices.push_back({-0.5f, 0.5f, 0.0f});
        vertices.push_back({0.0f, -0.5f, 0.0f});
        vertices.push_back({0.5f, 0.5f, 0.0f});
        bank.upload_buffer(vertex_buffer, vertices.data(), 0, sizeof(Vertex) * 3);
        is_init = true;
    }
    // Temporary
    draw_line({-0.5f, 0.5f, 0.0f}, {0.0f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    draw_line({0.0f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    draw_line({0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});

    /* Start a new imgui frame */
    imgui.new_frame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    /* End the imgui frame */
    ImGui::Render();

    render_graph.new_graph().unwrap();

    /* clang-format off */

    /* Test Rasterisation Pass */
    RasterNode& graphics_pass = render_graph.add_raster_pass("graphics pass", "test.vx", "test.px")
        .topology(Topology::TriangleList)
        .attribute(AttrFormat::XYZ32_SFloat)  // Position
        .load_op(LoadOp::Clear)
        .attach(render_target)
        .raster_extent(engine.window.width, engine.window.height);
    graphics_pass.draw(vertex_buffer, 3);

    /* Pipelines Enqueue */
    debug_pipeline.enqueue(render_graph);

    /* Add the immediate mode GUI to the render graph */
    render_graph.add_imgui(imgui, render_target);

    /* clang-format on */

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
    bank.destroy(render_target);
    bank.destroy(vertex_buffer);

    /* Pipelines Cleanup */
    debug_pipeline.deinit(gpu);

    /* Cleanup the VRAM bank & GPU adapter */
    render_graph.deinit().expect("failed to destroy render graph.");
    bank.deinit().expect("failed to destroy vram bank.");
    gpu.deinit().expect("failed to destroy gpu adapter.");
}

void Renderer::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color) { debug_pipeline.draw_line(start, end, color); }

}  // namespace tmt
