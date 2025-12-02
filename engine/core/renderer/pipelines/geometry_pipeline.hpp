#pragma once

#include <graphite/resources/handle.hh>

class GPUAdapter;
class RenderGraph;

namespace tmt {

constexpr uint32_t MAX_VOXEL_OBJECTS = 1;

class GeometryPipeline {
   private:
    Buffer object_buffer {};

   public:
    GeometryPipeline() {}
    ~GeometryPipeline() {}

    GeometryPipeline(const GeometryPipeline&) = delete;
    GeometryPipeline& operator=(const GeometryPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, Buffer render_view, RenderTarget render_target);
    void deinit(GPUAdapter& gpu);
};

}  // namespace tmt
