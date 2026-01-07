#pragma once

class RenderGraph;

namespace tmt {

struct RenderView;
struct SceneView;

/* Direct Illumination (DI) pipeline. */
class DiPipeline {
   private:
   public:
    DiPipeline() {}
    ~DiPipeline() {}

    DiPipeline(const DiPipeline&) = delete;
    DiPipeline& operator=(const DiPipeline&) = delete;

    void enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view);
};

}  // namespace tmt
