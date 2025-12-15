#pragma once

#include <vector>

#include <graphite/resources/handle.hh>

#include "core/renderer/render_view.hpp"
#include "events/engine.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

struct GpuPoint {
    glm::vec3 pos {0.0f, 0.0f, 0.0f};
    glm::vec3 color {1.0f, 0.0f, 0.0f};
};

struct DebugPoint {
    GpuPoint point {};
    float time = 0.0f;  // in seconds
};

// Should be 2x the amount of maximum lines (a line is made of 2 points)
constexpr uint32_t MAX_DEBUG_POINTS = 128;

class DebugPipeline : public OnEngineUpdate {
   private:
    Buffer point_buffer {};
    std::vector<DebugPoint> timed_points {};
    std::vector<DebugPoint> persistent_points {};

    uint32_t num_points = 0;

   public:
    DebugPipeline() {}
    ~DebugPipeline() override {}

    DebugPipeline(const DebugPipeline&) = delete;
    DebugPipeline& operator=(const DebugPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void on_engine_update(const FrameData& time) override;
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);

    void draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, const float time = 0.0f);
};
}  // namespace tmt