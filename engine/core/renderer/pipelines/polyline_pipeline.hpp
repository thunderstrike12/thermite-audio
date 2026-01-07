#pragma once

#include <vector>

#include <graphite/resources/handle.hh>

#include "engine/core/renderer/render_view.hpp"
#include "engine/events/engine.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

/* All data that makes up a polyline segment. */
struct PolylineSegment {
    glm::vec3 point_a {0.0f, 0.0f, 0.0f};
    glm::vec3 point_b {0.0f, 0.0f, 0.0f};
    glm::vec4 color {1.0f, 0.0f, 0.0f, 1.0f};
    float line_width = 0.1f;
    PolylineSegment() = default;
    PolylineSegment(glm::vec3 point_a, glm::vec3 point_b, glm::vec4 color, float line_width) : point_a(point_a), point_b(point_b), color(color), line_width(line_width) {};
};

/* Polyline segment with a timer attached. */
struct TimedPolylineSegment {
    PolylineSegment line {};
    float timer = 0.0f; /* Seconds */
    TimedPolylineSegment() = default;
    TimedPolylineSegment(glm::vec3 point_a, glm::vec3 point_b, glm::vec4 color, float line_width, float time) : line(point_a, point_b, color, line_width), timer(time) {};
};

/* Maximum number of polylines that can be drawn per frame. */
constexpr uint32_t MAX_POLYLINES = 1u << 19;

/**
 * Pipeline for drawing polygonal lines, used for debugging and editor visualizations.
 */
class PolylinePipeline : public OnEngineUpdate {
   private:
    /* GPU line segments buffer */
    Buffer line_buffer {};

    /* CPU buffers for timed and non-timed line segments */
    std::vector<TimedPolylineSegment> timed_lines {};
    std::vector<PolylineSegment> immediate_lines {};

    /* Number of active line segments */
    uint32_t line_segment_count = 0u;

   public:
    void init(GPUAdapter& gpu);
    void on_engine_update(const FrameData& time) override;
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);

    PolylinePipeline() = default;
    ~PolylinePipeline() = default;

    /* Cannot be copied */
    PolylinePipeline(const PolylinePipeline&) = delete;
    PolylinePipeline& operator=(const PolylinePipeline&) = delete;

    friend class Polyline;
};

}  // namespace tmt
