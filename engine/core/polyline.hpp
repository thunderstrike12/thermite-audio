#pragma once

#include <string_view>
#include <glm/gtc/quaternion.hpp>

namespace tmt {

/**
 * Polygonal line renderer interface.
 */
class Polyline {
    /* Polyline material state */
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float line_width = 1.5f;
    float depth_bias = 0.0f;
    bool logged_once = false;

   public:
    Polyline() = default;

    /* Use the given color when drawing subsequent lines. */
    inline void use_color(glm::vec3 new_color, float alpha = 1.0f) { color = glm::vec4(new_color, alpha); };
    inline void use_color(glm::vec4 new_color) { color = new_color; };
    inline void use_color(float r, float g, float b, float a = 1.0f) { color = glm::vec4(r, g, b, a); };
    /* Use the given line width when drawing subsequent lines. */
    inline void use_line_width(float new_width, bool screen_space = true) { line_width = screen_space ? abs(new_width) : -abs(new_width); };
    /* Use a depth bias for the lines, this will be added to their depth values. (mutually exclusive with `use_depth_testing`) */
    inline void use_depth_bias(float bias) { depth_bias = bias; };
    /* Enable/disable depth testing for the lines. */
    inline void use_depth_testing(bool value) { depth_bias = (value ? 0.0f : 1e30f); };

    /**
     * @brief Draw a polyline.
     * @param a The starting point of the polyline in world-space.
     * @param b The end point of the polyline in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_line(glm::vec3 a, glm::vec3 b, float time = 0.0f);

    /**
     * @brief Draw a polyline circle which always faces the camera.
     * @param origin The center point of the circle in world-space.
     * @param radius The radius of the circle in world-space.
     * @param segments The number of line segments which make up the circle.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_circle(glm::vec3 origin, float radius, uint32_t segments = 32u, float time = 0.0f);

    /**
     * @brief Draw a polyline circle in world-space.
     * @param origin The center point of the circle in world-space.
     * @param dir The direction of the circle in world-space.
     * @param radius The radius of the circle in world-space.
     * @param segments The number of line segments which make up the circle.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_world_circle(glm::vec3 origin, glm::vec3 dir, float radius, uint32_t segments = 32u, float time = 0.0f);

    /**
     * @brief Draw a polyline sphere using 3 orthogonal circles.
     * @param origin The center point of the sphere in world-space.
     * @param radius The radius of the sphere in world-space.
     * @param segments The number of line segments which make up each of the orthogonal circles.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_sphere(glm::vec3 origin, float radius, uint32_t segments = 32u, float time = 0.0f);

    /**
     * @brief Draw a polyline arrow which always faces the camera.
     * @param origin The origin point of the arrow in world-space.
     * @param dir The direction of the arrow in world-space.
     * @param length The length of the arrow in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_arrow(glm::vec3 origin, glm::vec3 dir, float length, float time = 0.0f);

    /**
     * @brief Draw a polyline axis aligned bounding box.
     * @param min The minimum point of the AABB in world-space.
     * @param max The maximum point of the AABB in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_aabb(glm::vec3 min, glm::vec3 max, float time = 0.0f);

    /**
     * @brief Draw a polyline oriented bounding box.
     * @param origin The center point of the OBB in world-space.
     * @param half_extent Half of the extent of the OBB in local-space using world-space units.
     * @param rot The rotation of the OBB in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_obb(glm::vec3 origin, glm::vec3 half_extent, glm::quat rot, float time = 0.0f);

    /**
     * @brief Draw a polyline cone with side lines that always face the camera.
     * @param origin The origin point of the cone in world-space.
     * @param dir The direction of the cone in world-space.
     * @param angle The half angle of the cone in radians.
     * @param length The length of the cone in world-space.
     * @param segments The number of line segments which make up the circle at the end of the cone.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_cone(glm::vec3 origin, glm::vec3 dir, float angle, float length, uint32_t segments = 32u, float time = 0.0f);

    /**
     * @brief Draw a polyline tube with side lines that always face the camera.
     * @param a The starting point of the tube in world-space.
     * @param b The end point of the tube in world-space.
     * @param radius The radius of the tube in world-space.
     * @param segments The number of line segments which make up each of the end circles.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_tube(glm::vec3 a, glm::vec3 b, float radius, uint32_t segments = 32u, float time = 0.0f);

    /**
     * @brief Draw a polyline animation bone which only shows the lines facing the camera.
     * @param origin The origin (bottom) of the bone in world-space.
     * @param rot The rotation of the bone in world-space.
     * @param length The length of the bone in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_bone(glm::vec3 origin, glm::quat rot, float length, float time = 0.0f);

    /**
     * @brief Draw a polyline string of text always facing the camera.
     * @param origin The origin of the text in world-space.
     * @param text The string to display.
     * @param scale The scale of the text in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_text(glm::vec3 origin, std::string_view text, float scale, float time = 0.0f);

    /**
     * @brief Draw a polyline scene grid with axis lines.
     * @param cell_size Size of a grid cell in world-space.
     * @param subdivisions Every N lines, draw a major line.
     * @param grid_extent Extent of the grid in cells.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_scene_grid(float cell_size, int subdivisions, int grid_extent, float time = 0.0f);
};

}  // namespace tmt
