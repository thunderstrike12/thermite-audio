#pragma once

#include <graphite/vram_bank.hh>
#include <graphite/imgui.hh>

class GPUAdapter;
class RenderGraph;

namespace tmt {
class Renderer {
   public:
    GPUAdapter& gpu;
    RenderGraph& render_graph;
    RenderTarget render_target {};
    ImGUI imgui {};

    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void update();
    void end();
};
}  // namespace tmt