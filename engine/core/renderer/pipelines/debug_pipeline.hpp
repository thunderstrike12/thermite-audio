#pragma once

#include <vector>

#include <graphite/resources/handle.hh>

class GPUAdapter;
class RenderGraph;

namespace tmt {

struct DebugPoint {
    glm::vec3 start {0.0f, 0.0f, 0.0f};
    glm::vec3 color {1.0f, 0.0f, 0.0f};
};

constexpr uint32_t MAX_DEBUG_POINTS = 32;

class DebugPipeline {
   private:
    Buffer point_buffer {};
    std::vector<DebugPoint> debug_points {};
    u32 num_points = 0;

   public:
    DebugPipeline() {}
    ~DebugPipeline() {}

    DebugPipeline(const DebugPipeline&) = delete;
    DebugPipeline& operator=(const DebugPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderTarget render_target);
    void deinit(GPUAdapter& gpu);

    void draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color = {1.0f, 0.0f, 0.0f});
};
}  // namespace tmt