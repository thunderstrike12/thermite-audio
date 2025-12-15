#pragma once

#include <vector>

#include <glm/gtc/quaternion.hpp>

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
constexpr uint32_t MAX_DEBUG_POINTS = 1024;

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

    /* Draw a circle in 3D space. Axis_a and axis_b define the plane that the circle lays in */
    void draw_circle(
        const glm::vec3 center, const float radius, const glm::vec3 axis_a, const glm::vec3 axis_b, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, int segments = 16, const float time = 0.0f
    );

    /* Needs at least 4 segments and 2 rings */
    void draw_sphere(const glm::vec3 center, const float radius, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, int rings = 4, int segments = 16, const float time = 0.0f);

    /* head_angle is in degrees */
    void draw_arrow(
        const glm::vec3 start, glm::vec3 dir, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, float length = 1.0f, float head_length = 0.1f, float head_angle = 10.0f, const float time = 0.0f
    );

    void draw_cross(const glm::vec3 center, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, const float size = 0.5f, const glm::quat rot = glm::quat(), const float time = 0.0f);

    /* If rot is not set, we draw an aabb */
    void draw_obb(
        const glm::vec3 center, const float width, const float height, const float depth, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, const glm::quat rot = glm::quat(), const float time = 0.0f
    );
};
}  // namespace tmt