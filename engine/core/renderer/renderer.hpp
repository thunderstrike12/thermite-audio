#pragma once

#include <vector>

#include <graphite/imgui.hh>
#include <graphite/resources/handle.hh>

#include "core/ecs.hpp"
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
    RenderTarget render_target {};
    RenderView render_view {};
    Buffer render_view_buffer {};

    ImGUI imgui {};

    /* Pipelines */
    DebugPipeline& debug_pipeline;
    GeometryPipeline& geometry_pipeline;

   public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void update();
    void end();

    void draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color = {1.0f, 0.0f, 0.0f});
};

}  // namespace tmt
