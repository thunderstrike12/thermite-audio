#include "polyline.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>

#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/pipelines/polyline_pipeline.hpp"
#include "engine/core/logger.hpp"

namespace tmt {

/* Internal polyline pipeline quick access. */
PolylinePipeline& polyline = engine.renderer.polyline_pipeline;

void Polyline::draw_line(glm::vec3 a, glm::vec3 b, float time) {
    /* Check if we reached the maximum number of polylines */
    if (polyline.line_segment_count >= MAX_POLYLINES - 1u) {
        if (logged_once == false) {
            Log::warn(Log::Scope::RENDERER, "Max amount of polylines reached. Consider increasing MAX_POLYLINES, or draw less lines.");
            logged_once = true;
        }
        return;
    }

    /* Increment the line segment count */
    polyline.line_segment_count += 1u;

    /* If the time is 0 that means this is an immediate line */
    if (time == 0.0f) {
        polyline.immediate_lines.emplace_back(a, b, color, line_width);
        return;
    }

    /* If the time *is* set, create a timed line */
    polyline.timed_lines.emplace_back(a, b, color, line_width, time);
}

void Polyline::draw_circle(glm::vec3 origin, float radius, uint32_t segments, float time) {
    /* Less than 4 segments doesn't make sense */
    if (segments < 4u) segments = 4u;

    /* Find the direction from the origin of the circle to the camera */
    const glm::vec3 camera_pos = engine.renderer.render_view.gpu_view.origin;
    const glm::vec3 camera_dir = glm::normalize(camera_pos - origin);

    /* Pick a reference vector that isn't parallel to camera_dir */
    const glm::vec3 ref = glm::abs(glm::dot(camera_dir, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    /* Find the two axis on which the circle should be drawn */
    const glm::vec3 axis_a = glm::normalize(glm::cross(camera_dir, ref));
    const glm::vec3 axis_b = glm::cross(camera_dir, axis_a);

    /* Finally draw the line segments which make up the circle */
    const float step = glm::two_pi<float>() / static_cast<float>(segments);
    for (uint32_t i = 0u; i < segments; ++i) {
        const float a0 = step * static_cast<float>(i);
        const float a1 = step * static_cast<float>(i + 1u);
        const glm::vec3 p0 = origin + radius * (axis_a * glm::cos(a0) + axis_b * glm::sin(a0));
        const glm::vec3 p1 = origin + radius * (axis_a * glm::cos(a1) + axis_b * glm::sin(a1));
        draw_line(p0, p1, time);
    }
}

void Polyline::draw_sphere(glm::vec3 origin, float radius, uint32_t segments, float time) {
    /* Less than 4 segments doesn't make sense */
    if (segments < 4u) segments = 4u;
    const float step = glm::two_pi<float>() / static_cast<float>(segments);

    /* XY plane circle (Z axis) */
    for (uint32_t i = 0u; i < segments; ++i) {
        const float a0 = step * static_cast<float>(i);
        const float a1 = step * static_cast<float>(i + 1u);
        const glm::vec3 p0 = origin + radius * glm::vec3(glm::cos(a0), glm::sin(a0), 0.0f);
        const glm::vec3 p1 = origin + radius * glm::vec3(glm::cos(a1), glm::sin(a1), 0.0f);
        draw_line(p0, p1, time);
    }

    /* XZ plane circle (Y axis) */
    for (uint32_t i = 0u; i < segments; ++i) {
        const float a0 = step * static_cast<float>(i);
        const float a1 = step * static_cast<float>(i + 1u);
        const glm::vec3 p0 = origin + radius * glm::vec3(glm::cos(a0), 0.0f, glm::sin(a0));
        const glm::vec3 p1 = origin + radius * glm::vec3(glm::cos(a1), 0.0f, glm::sin(a1));
        draw_line(p0, p1, time);
    }

    /* YZ plane circle (X axis) */
    for (uint32_t i = 0u; i < segments; ++i) {
        const float a0 = step * static_cast<float>(i);
        const float a1 = step * static_cast<float>(i + 1u);
        const glm::vec3 p0 = origin + radius * glm::vec3(0.0f, glm::cos(a0), glm::sin(a0));
        const glm::vec3 p1 = origin + radius * glm::vec3(0.0f, glm::cos(a1), glm::sin(a1));
        draw_line(p0, p1, time);
    }
}

void Polyline::draw_arrow(glm::vec3 origin, glm::vec3 dir, float length, float time) {
    /* Draw the main line */
    const glm::vec3 tip = origin + dir * length;
    draw_line(origin, tip, time);

    /* Find the direction from the origin of the arrow to the camera */
    const glm::vec3 camera_pos = engine.renderer.render_view.gpu_view.origin;
    const glm::vec3 camera_dir = glm::normalize(camera_pos - tip);

    /* Find perpendicular axis that faces the camera */
    glm::vec3 perp = glm::cross(dir, camera_dir);
    if (glm::length2(perp) < 0.0001f) {
        glm::vec3 ref = glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(dir, ref)) > 0.99f) {
            ref = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        perp = glm::cross(dir, ref);
    }
    perp = glm::normalize(perp);

    /* Arrow head constants */
    const float head_length = length * 0.30f;
    const float head_width = length * 0.12f;
    const glm::vec3 head_base = tip - dir * head_length;

    /* Draw arrow fins */
    draw_line(tip, head_base + perp * head_width, time);
    draw_line(tip, head_base - perp * head_width, time);
}

void Polyline::draw_aabb(glm::vec3 min, glm::vec3 max, float time) {
    const glm::vec3 corners[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}, {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z},
    };

    /* Bottom face */
    draw_line(corners[0], corners[1], time);
    draw_line(corners[1], corners[2], time);
    draw_line(corners[2], corners[3], time);
    draw_line(corners[3], corners[0], time);

    /* Top face */
    draw_line(corners[4], corners[5], time);
    draw_line(corners[5], corners[6], time);
    draw_line(corners[6], corners[7], time);
    draw_line(corners[7], corners[4], time);

    /* Vertical edges */
    draw_line(corners[0], corners[4], time);
    draw_line(corners[1], corners[5], time);
    draw_line(corners[2], corners[6], time);
    draw_line(corners[3], corners[7], time);
}

void Polyline::draw_obb(glm::vec3 origin, glm::vec3 half_extent, glm::quat rot, float time) {
    /* Find the 8 corners of the OBB */
    const glm::vec3 ax = rot * glm::vec3(half_extent.x, 0.0f, 0.0f);
    const glm::vec3 ay = rot * glm::vec3(0.0f, half_extent.y, 0.0f);
    const glm::vec3 az = rot * glm::vec3(0.0f, 0.0f, half_extent.z);
    const glm::vec3 corners[8] = {
        origin - ax - ay - az, origin + ax - ay - az, origin + ax + ay - az, origin - ax + ay - az, origin - ax - ay + az, origin + ax - ay + az, origin + ax + ay + az, origin - ax + ay + az,
    };

    /* Bottom face */
    draw_line(corners[0], corners[1], time);
    draw_line(corners[1], corners[2], time);
    draw_line(corners[2], corners[3], time);
    draw_line(corners[3], corners[0], time);

    /* Top face */
    draw_line(corners[4], corners[5], time);
    draw_line(corners[5], corners[6], time);
    draw_line(corners[6], corners[7], time);
    draw_line(corners[7], corners[4], time);

    /* Vertical edges */
    draw_line(corners[0], corners[4], time);
    draw_line(corners[1], corners[5], time);
    draw_line(corners[2], corners[6], time);
    draw_line(corners[3], corners[7], time);
}

void Polyline::draw_cone(glm::vec3 origin, glm::vec3 dir, float angle, float length, uint32_t segments, float time) {
    const glm::vec3 base_center = origin + dir * length;
    const float radius = length * glm::tan(angle);

    /* Find perpendicular axes for the base circle */
    glm::vec3 ref = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(dir, ref)) > 0.99f) {
        ref = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    const glm::vec3 axis_a = glm::normalize(glm::cross(dir, ref));
    const glm::vec3 axis_b = glm::cross(dir, axis_a);

    /* Draw the base circle */
    if (segments < 4u) segments = 4u;
    const float step = glm::two_pi<float>() / static_cast<float>(segments);
    for (uint32_t i = 0u; i < segments; ++i) {
        const float a0 = step * static_cast<float>(i);
        const float a1 = step * static_cast<float>(i + 1u);
        const glm::vec3 p0 = base_center + radius * (axis_a * glm::cos(a0) + axis_b * glm::sin(a0));
        const glm::vec3 p1 = base_center + radius * (axis_a * glm::cos(a1) + axis_b * glm::sin(a1));
        draw_line(p0, p1, time);
    }

    /* Camera facing lines from the tip to the base of the cone */
    const glm::vec3 camera_pos = engine.renderer.render_view.gpu_view.origin;
    const glm::vec3 to_camera = glm::normalize(camera_pos - base_center);

    glm::vec3 perp = glm::cross(dir, to_camera);
    if (glm::length2(perp) < 0.0001f) {
        perp = axis_a;
    } else {
        perp = glm::normalize(perp);
    }

    draw_line(origin, base_center + perp * radius, time);
    draw_line(origin, base_center - perp * radius, time);
}

void Polyline::draw_tube(glm::vec3 a, glm::vec3 b, float radius, uint32_t segments, float time) {
    const glm::vec3 dir = glm::normalize(b - a);

    /* Find perpendicular axes for the end circles */
    glm::vec3 ref = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(dir, ref)) > 0.99f) {
        ref = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    const glm::vec3 axis_a = glm::normalize(glm::cross(dir, ref));
    const glm::vec3 axis_b = glm::cross(dir, axis_a);

    /* Draw the end circles */
    if (segments < 4u) segments = 4u;
    const float step = glm::two_pi<float>() / static_cast<float>(segments);
    for (uint32_t i = 0u; i < segments; ++i) {
        const float a0 = step * static_cast<float>(i);
        const float a1 = step * static_cast<float>(i + 1u);
        const glm::vec3 offset0 = radius * (axis_a * glm::cos(a0) + axis_b * glm::sin(a0));
        const glm::vec3 offset1 = radius * (axis_a * glm::cos(a1) + axis_b * glm::sin(a1));
        draw_line(a + offset0, a + offset1, time);
        draw_line(b + offset0, b + offset1, time);
    }

    /* Camera facing side lines of the tube */
    const glm::vec3 center = (a + b) * 0.5f;
    const glm::vec3 camera_pos = engine.renderer.render_view.gpu_view.origin;
    const glm::vec3 to_camera = glm::normalize(camera_pos - center);

    glm::vec3 perp = glm::cross(dir, to_camera);
    if (glm::length2(perp) < 0.0001f) {
        perp = axis_a;
    } else {
        perp = glm::normalize(perp);
    }

    draw_line(a + perp * radius, b + perp * radius, time);
    draw_line(a - perp * radius, b - perp * radius, time);
}

void Polyline::draw_bone(glm::vec3 origin, glm::quat rot, float length, float time) {
    if (length < 0.0001f) return;

    /* Mid-point and width inferred from dir and length */
    const glm::vec3 dir = rot * glm::vec3(0, 1, 0);
    const glm::vec3 mid = origin + dir * length * 0.125f;
    const float width = length * 0.125f;
    const glm::vec3 b = origin + dir * length;

    /* Get the four corner points of the bone */
    const glm::vec3 rot_a = rot * glm::vec3(1, 0, 0) * width;
    const glm::vec3 rot_b = rot * glm::vec3(0, 0, 1) * width;
    const glm::vec3 points[4] = {mid + rot_a, mid + rot_b, mid - rot_a, mid - rot_b};
    const glm::vec3 to_cam = glm::normalize(glm::vec3(engine.renderer.render_view.gpu_view.origin) - mid);

    /* Draw each of the bone edges (only if they would be visible if it was solid) */
    for (int i = 0; i < 4; ++i) {
        const int n = (i + 1) & 3;
        const int pr = (i + 3) & 3;

        const bool front_i = glm::dot(glm::cross(points[i] - origin, points[n] - origin), to_cam) > 0.0f;
        const bool front_pr = glm::dot(glm::cross(points[pr] - origin, points[i] - origin), to_cam) > 0.0f;
        const bool back_i = glm::dot(glm::cross(points[n] - b, points[i] - b), to_cam) > 0.0f;
        const bool back_pr = glm::dot(glm::cross(points[i] - b, points[pr] - b), to_cam) > 0.0f;

        if (front_i || front_pr) draw_line(origin, points[i], time);
        if (back_i || back_pr) draw_line(points[i], b, time);
        if (front_i || back_i) draw_line(points[i], points[n], time);
    }
}

}  // namespace tmt
