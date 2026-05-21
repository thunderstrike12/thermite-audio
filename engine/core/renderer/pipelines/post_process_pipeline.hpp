#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/renderer/render_view.hpp"
#include "engine/core/renderer/scene_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

struct ChromaticAberration {
    glm::vec3 rgb_offsets = glm::vec3(0.01f, 0.0f, -0.01f);
    float strength = 0.5f;
};

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

    ChromaticAberration ca_values;
};

}  // namespace tmt

TMT_OBJECT(tmt::ChromaticAberration, (rgb_offsets, strength));
TMT_OBJECT(tmt::PostProcessPipeline, (ca_values));