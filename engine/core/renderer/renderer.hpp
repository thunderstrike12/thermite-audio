#pragma once

#include <vector>

#include <graphite/imgui.hh>

#include "render_view.hpp"

class GPUAdapter;
class RenderGraph;
class VRAMBank;

namespace tmt {

class DebugPipeline;
class GeometryPipeline;

class Renderer {
    GPUAdapter& gpu;
    RenderGraph& render_graph;

    /* Pipelines */
    DebugPipeline& debug_pipeline;
    GeometryPipeline& geometry_pipeline;

   public:
    RenderView render_view {};
    ImGUI* imgui = nullptr;

    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void update();
    void end();

    void draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color = {1.0f, 0.0f, 0.0f});

    VRAMBank& vram_bank();

    void set_imgui(ImGUI* imgui, ImGUIFunctions functions);
};

}  // namespace tmt
