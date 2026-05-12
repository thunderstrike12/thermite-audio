#pragma once

#include <graphite/resources/handle.hh>

class RenderGraph;
class GPUAdapter;

namespace tmt {

struct RenderView;
struct SceneView;

constexpr uint32_t CACHE_SIZE = 10000000u;

/* Lighting pipeline. */
class LightingPipeline {
   public:
    LightingPipeline() {}
    ~LightingPipeline() {}

    LightingPipeline(const LightingPipeline&) = delete;
    LightingPipeline& operator=(const LightingPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view);
    void deinit(GPUAdapter& gpu);

    bool cache_init = false;
};

}  // namespace tmt
