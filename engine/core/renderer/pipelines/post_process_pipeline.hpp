#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/renderer/render_view.hpp"
#include "engine/core/renderer/scene_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

class PostProcessPipeline {
   public:
    PostProcessPipeline() {}
    ~PostProcessPipeline() {}

    PostProcessPipeline(const PostProcessPipeline&) = delete;
    PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view);
    void deinit(GPUAdapter& gpu);

    Sampler lut_sampler;
};

}  // namespace tmt