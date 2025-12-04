#pragma once

#include <graphite/resources/handle.hh>

#include "engine/shared/bvh2.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

struct VoxelObject;

constexpr uint32_t MAX_VOXEL_OBJECTS = 256u;

class GeometryPipeline {
   private:
    /* GPU resources */
    Buffer bvh_nodes {};
    Buffer object_indices {};
    Buffer object_data {};

    /* Acceleration structure */
    Bvh2<VoxelObject> bvh {};

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
