#pragma once

class RenderGraph;

namespace tmt {

struct RenderView;
struct SceneView;

/* Geometry / Visibility pipeline. */
class GeometryPipeline {
   public:
    GeometryPipeline() {}
    ~GeometryPipeline() {}

    GeometryPipeline(const GeometryPipeline&) = delete;
    GeometryPipeline& operator=(const GeometryPipeline&) = delete;

    void enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view);
};

}  // namespace tmt
