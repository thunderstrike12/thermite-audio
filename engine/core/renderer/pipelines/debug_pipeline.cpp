#include "debug_pipeline.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>

#include "engine.hpp"
#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

void DebugPipeline::init(GPUAdapter& gpu) {
    /* Get VRAM Bank */
    VRAMBank& bank = gpu.get_vram_bank();

    /* Initialize the Line Buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Vertex | BufferUsage::TransferDst, MAX_DEBUG_POINTS, sizeof(GpuPoint)); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialise line buffer.\nreason: {}", r.unwrap_err().c_str());
        return;
    } else
        point_buffer = r.unwrap();
}

void DebugPipeline::on_engine_update(const FrameData& time) {
    if (!timed_points.empty()) {
        for (auto& p : timed_points) p.time -= time.delta_time;

        std::erase_if(timed_points, [](const DebugPoint& p) { return p.time <= 0.0f; });
    }

    num_points = timed_points.size() + persistent_points.size();
}

void DebugPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();

    /* Upload Point Buffer */
    if (num_points > 0) {
        std::vector<GpuPoint> packed {};

        for (int i = 0; i < timed_points.size(); i++) {
            packed.push_back(timed_points[i].point);
        }
        for (int j = 0; j < persistent_points.size(); j++) {
            packed.push_back(persistent_points[j].point);
        }

        render_graph.upload_buffer(point_buffer, packed.data(), 0, sizeof(GpuPoint) * num_points);
    }

    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    RasterNode& line_pass = render_graph.add_raster_pass("debug line pass", "debug_line.vx", "debug_line.px")
                                .topology(Topology::LineList)
                                .attribute(AttrFormat::XYZ32_SFloat)  // Position
                                .attribute(AttrFormat::XYZ32_SFloat)  // Color
                                .read(render_view.render_view_buffer, ShaderStages::Vertex)
                                .attach(render_image)
                                .raster_extent(render_res.x, render_res.y);
    line_pass.draw(point_buffer, num_points);

    // Update Points
    num_points -= persistent_points.size();
    persistent_points.clear();
}

void DebugPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    bank.destroy(point_buffer);
}

void DebugPipeline::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color, const float time) {
    if (num_points > MAX_DEBUG_POINTS - 2) {
        Log::error(Log::Scope::RENDERER, "max amount of lines reached. Consider increasing MAX_DEBUG_POINTS, or draw less lines.");
        return;
    }

    // Timed Lines
    if (time > 0.0f) {
        timed_points.push_back({{start, color}, time});
        timed_points.push_back({{end, color}, time});
    } else {  // Persistent Lines
        persistent_points.push_back({{start, color}, time});
        persistent_points.push_back({{end, color}, time});
    }
    num_points += 2;
}

void DebugPipeline::draw_circle(const glm::vec3 center, const float radius, const glm::vec3 axis_a, const glm::vec3 axis_b, const glm::vec3 color, int segments, const float time) {
    if (segments < 4) segments = 4;

    const float step = glm::two_pi<float>() / static_cast<float>(segments);

    glm::vec3 prev = center + radius * (axis_a * cosf(0.0f) + axis_a * sinf(0.0f));

    for (int i = 1; i <= segments; ++i) {
        float angle = step * i;

        glm::vec3 curr = center + radius * (axis_a * cosf(angle) + axis_b * sinf(angle));

        draw_line(prev, curr, color, time);
        prev = curr;
    }
}

void DebugPipeline::draw_sphere(const glm::vec3 center, float radius, const glm::vec3 color, int rings, int segments, const float time) {
    if (segments < 4) segments = 4;
    if (rings < 2) rings = 2;

    const int latitude_rings = rings / 2;
    const int longitude_rings = rings - latitude_rings;

    // Great circles (always draw these)
    draw_circle(center, radius, {1, 0, 0}, {0, 1, 0}, color, segments, time);
    draw_circle(center, radius, {1, 0, 0}, {0, 0, 1}, color, segments, time);
    draw_circle(center, radius, {0, 1, 0}, {0, 0, 1}, color, segments, time);

    // Latitude rings (horizontal)
    for (int i = 1; i <= latitude_rings; ++i) {
        const float t = float(i) / float(latitude_rings + 1);
        const float y = glm::mix(-radius, radius, t);

        const float ring_radius = sqrtf(radius * radius - y * y);

        draw_circle(center + glm::vec3(0, y, 0), ring_radius, {1, 0, 0}, {0, 0, 1}, color, segments, time);
    }

    // Longitude rings (vertical)
    for (int i = 0; i <= longitude_rings; ++i) {
        const float angle = glm::two_pi<float>() * float(i + 0.5f) / float(longitude_rings);

        const glm::vec3 axis_a = {cosf(angle), 0.0f, sinf(angle)};

        const glm::vec3 axis_b = {0.0f, 1.0f, 0.0f};

        draw_circle(center, radius, axis_a, axis_b, color, segments, time);
    }
}

void DebugPipeline::draw_arrow(const glm::vec3 start, glm::vec3 dir, const glm::vec3 color, float length, float head_length, float head_angle, const float time) {
    if (glm::length2(dir) < 1e-6f) return;

    dir = glm::normalize(dir);
    const glm::vec3 end = start + dir * length;

    // Shaft
    draw_line(start, end, color, time);

    // Arrow head
    const float head_len = length * head_length;
    const float angle_rad = glm::radians(head_angle);

    // Build a perpendicular basis
    glm::vec3 up = fabs(dir.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);

    const glm::vec3 right = glm::normalize(glm::cross(dir, up));
    up = glm::normalize(glm::cross(right, dir));

    // Two head lines
    const glm::vec3 head_dir1 = glm::normalize(dir * cosf(angle_rad) + right * sinf(angle_rad));
    const glm::vec3 head_dir2 = glm::normalize(dir * cosf(angle_rad) - right * sinf(angle_rad));

    draw_line(end, end - head_dir1 * head_len, color, time);
    draw_line(end, end - head_dir2 * head_len, color, time);

    // Add vertical lines for a 3D look
    const glm::vec3 head_dir3 = glm::normalize(dir * cosf(angle_rad) + up * sinf(angle_rad));
    const glm::vec3 head_dir4 = glm::normalize(dir * cosf(angle_rad) - up * sinf(angle_rad));

    draw_line(end, end - head_dir3 * head_len, color, time);
    draw_line(end, end - head_dir4 * head_len, color, time);
}

void DebugPipeline::draw_cross(const glm::vec3 center, const glm::vec3 color, const float size, const glm::quat rot, const float time) {
    static const glm::quat default_rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1));

    const glm::quat final_rotation = (rot == glm::quat()) ? default_rotation : rot;

    // Base axes (local cross)
    const glm::vec3 x = glm::vec3(size, 0, 0);
    const glm::vec3 y = glm::vec3(0, size, 0);

    // Rotate axes
    const glm::vec3 rx = final_rotation * x;
    const glm::vec3 ry = final_rotation * y;

    // Draw lines
    draw_line(center - rx, center + rx, color, time);
    draw_line(center - ry, center + ry, color, time);
}

void DebugPipeline::draw_obb(const glm::vec3 center, const float width, const float height, const float depth, const glm::vec3 color, const glm::quat rot, const float time) {
    const glm::vec3 half = glm::vec3(width, height, depth) * 0.5f;

    // Local corners (before rotation)
    glm::vec3 p0 = glm::vec3(-half.x, -half.y, -half.z);
    glm::vec3 p1 = glm::vec3(half.x, -half.y, -half.z);
    glm::vec3 p2 = glm::vec3(half.x, -half.y, half.z);
    glm::vec3 p3 = glm::vec3(-half.x, -half.y, half.z);

    glm::vec3 p4 = glm::vec3(-half.x, half.y, -half.z);
    glm::vec3 p5 = glm::vec3(half.x, half.y, -half.z);
    glm::vec3 p6 = glm::vec3(half.x, half.y, half.z);
    glm::vec3 p7 = glm::vec3(-half.x, half.y, half.z);

    // Rotate and translate corners
    p0 = center + rot * p0;
    p1 = center + rot * p1;
    p2 = center + rot * p2;
    p3 = center + rot * p3;
    p4 = center + rot * p4;
    p5 = center + rot * p5;
    p6 = center + rot * p6;
    p7 = center + rot * p7;

    // Bottom face
    draw_line(p0, p1, color, time);
    draw_line(p1, p2, color, time);
    draw_line(p2, p3, color, time);
    draw_line(p3, p0, color, time);

    // Top face
    draw_line(p4, p5, color, time);
    draw_line(p5, p6, color, time);
    draw_line(p6, p7, color, time);
    draw_line(p7, p4, color, time);

    // Vertical edges
    draw_line(p0, p4, color, time);
    draw_line(p1, p5, color, time);
    draw_line(p2, p6, color, time);
    draw_line(p3, p7, color, time);
}

}  // namespace tmt