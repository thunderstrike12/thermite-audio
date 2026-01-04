#pragma once

#include <vector>

#include <glm/gtc/quaternion.hpp>

#include "render_view.hpp"

class GPUAdapter;
class RenderGraph;
class VRAMBank;
class ImGUI;

namespace tmt {

class PolylinePipeline;
class GeometryPipeline;

class Renderer {
    GPUAdapter& gpu;
    RenderGraph& render_graph;

    Transform debug_transform;
    Camera debug_camera;

   public:
    /* Pipelines */
    PolylinePipeline& polyline_pipeline;
    GeometryPipeline& geometry_pipeline;

    RenderView render_view {};
#ifdef THERMITE_EDITOR
    ImGUI* imgui = nullptr;
#endif

    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void update();
    void end();

    VRAMBank& vram_bank();

    Transform& get_debug_transform() { return debug_transform; }
    Camera& get_debug_camera() { return debug_camera; }

#ifdef THERMITE_EDITOR
    void set_imgui(ImGUI* imgui);
#endif
};

}  // namespace tmt
