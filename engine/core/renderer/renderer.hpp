#pragma once

#include <vector>

#include <graphite/imgui.hh>
#include <graphite/resources/handle.hh>

#include "render_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

class DebugPipeline;
class GeometryPipeline;

class Renderer {
    GPUAdapter& gpu;
    RenderGraph& render_graph;

    /* Renderer output */
    Texture viewport_texture {};
    Image viewport_image {};
    u64 imgui_viewport {};
    RenderTarget render_target {};
    Buffer render_view_buffer {};

    ImGUI* imgui = nullptr;

    /* Pipelines */
    DebugPipeline& debug_pipeline;
    GeometryPipeline& geometry_pipeline;

   public:
    RenderView render_view {};

    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void update();
    void end();

    /* If we are running the editor, we render to the viewport_image, otherwise we render directly to the render_target */
    BindHandle get_render_image() const;

    u64 get_imgui_viewport() const;
    void set_viewport_size(uint32_t width, uint32_t height);

    void draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color = {1.0f, 0.0f, 0.0f});

    void set_imgui(ImGUI* imgui, ImGUIFunctions functions);
};

}  // namespace tmt
