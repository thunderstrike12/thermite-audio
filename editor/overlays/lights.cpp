#include "lights.hpp"

#include "editor/editor.hpp"
#include "editor/shared/colors.hpp"
#include "editor/windows/hierarchy.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/light.hpp"

namespace tmt {

/* Size of the reticle used for overlays. (diamond shape in the center) */
constexpr float OVERLAY_RETICLE_SIZE = 0.01f;
/* Fading radius for sphere light radii. */
constexpr float SPHERE_FADE_RADIUS = 8.0f;

/* Draw sphere light overlay. */
inline void sphere_light_overlay(Transform& transform, SphereLight light, bool selected) {
    /* Get the position of the light source */
    const glm::vec3 pos = transform.get_world_position();
    const glm::vec3 color = selected ? colors::SELECTED : colors::DARK;
    const glm::vec3 camera_pos = engine.renderer.render_view.gpu_view.origin;
    const float camera_dist = glm::distance(camera_pos, pos);

    /* Draw the radii */
    if (camera_dist > light.source_radius) {
        engine.polyline.use_color(color, (camera_dist - light.source_radius) / SPHERE_FADE_RADIUS);
        engine.polyline.draw_circle(pos, light.source_radius, 32u); /* Source radius */
    }
    if (camera_dist > light.attenuation_radius) {
        engine.polyline.use_color(color, (camera_dist - light.attenuation_radius) / SPHERE_FADE_RADIUS);
        engine.polyline.draw_circle(pos, light.attenuation_radius, 32u); /* Attenuation radius */
    }

    /* Draw vertical line */
    engine.polyline.use_color(color, 0.5f);
    engine.polyline.draw_line(pos, pos * glm::vec3(1.0f, 0.0f, 1.0f));
}

/* Number of lines used to indicate a sun light overlay. */
constexpr uint32_t SUN_LINES = 8u;
/* Padding of the lines used to indicate a sun light overlay. */
constexpr float SUN_LINES_PADDING = 0.004f;
/* Length of the lines used to indicate a sun light overlay. */
constexpr float SUN_LINES_LENGTH = 0.0075f;
/* Length of the line used to indicate the sun light direction. */
constexpr float SUN_INDICATOR_LENGTH = 10.0f;

/* Draw sun light overlay. */
inline void sun_light_overlay(Transform& transform, bool selected) {
    /* Get the position of the light source */
    const glm::vec3 pos = transform.get_world_position();
    const glm::vec3 color = selected ? colors::SELECTED : colors::DARK;
    engine.polyline.use_color(color);

    /* Calculate the camera position, direction, and distance */
    const glm::vec3 camera_pos = engine.renderer.render_view.gpu_view.origin;
    const glm::vec3 camera_dir = glm::normalize(camera_pos - pos);
    const float camera_dist = glm::distance(camera_pos, pos);

    /* Pick a reference vector that isn't parallel to camera_dir */
    const glm::vec3 ref = glm::abs(glm::dot(camera_dir, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    /* Find the two axis on which the circle should be drawn */
    const glm::vec3 axis_a = glm::normalize(glm::cross(camera_dir, ref));
    const glm::vec3 axis_b = glm::cross(camera_dir, axis_a);

    /* Finally draw the line segments which make up the circle */
    const float step = glm::two_pi<float>() / static_cast<float>(SUN_LINES);
    const float inner_radius = camera_dist * (OVERLAY_RETICLE_SIZE + 0.004f);
    const float outer_radius = camera_dist * (OVERLAY_RETICLE_SIZE + 0.004f + 0.0075f);
    for (uint32_t i = 0u; i < SUN_LINES; ++i) {
        const float a = step * static_cast<float>(i);
        const glm::vec3 p0 = pos + inner_radius * (axis_a * glm::cos(a) + axis_b * glm::sin(a));
        const glm::vec3 p1 = pos + outer_radius * (axis_a * glm::cos(a) + axis_b * glm::sin(a));
        engine.polyline.draw_line(p0, p1);
    }

    /* Draw forward facing line */
    engine.polyline.draw_line(pos, pos + transform.get_forward() * SUN_INDICATOR_LENGTH);

    /* Draw vertical line */
    engine.polyline.use_color(color, 0.5f);
    engine.polyline.draw_line(pos, pos * glm::vec3(1.0f, 0.0f, 1.0f));
}

/* Base cone size for the spot light overlay. */
constexpr float SPOTLIGHT_CONE_SIZE = 10.0f;

/* Draw spot light overlay. */
inline void spot_light_overlay(Transform& transform, const SpotLight& light, bool selected) {
    /* Get the position of the light source */
    const glm::vec3 pos = transform.get_world_position();
    const glm::vec3 dir = transform.get_forward();
    const glm::vec3 color = selected ? colors::SELECTED : colors::DARK;

    /* Draw the source radius */
    engine.polyline.use_color(color);
    engine.polyline.draw_circle(pos, light.source_radius, 32u); /* Source radius */

    /* Draw the attenuation distance using a line */
    engine.polyline.use_color(color, 0.5f);
    engine.polyline.draw_line(pos, pos + dir * light.attenuation_distance); /* Attenuation distance */

    /* Find the cone radius and length based on the cone size and angle */
    const float safe_angle = glm::tan(glm::min(light.beam_angle * 0.5f, glm::pi<float>() * 0.5f - 0.001f));
    const float cone_radius = SPOTLIGHT_CONE_SIZE * glm::sin(light.beam_angle * 0.5f);
    const float cone_length = glm::max(0.01f, glm::sqrt(SPOTLIGHT_CONE_SIZE * SPOTLIGHT_CONE_SIZE - cone_radius * cone_radius));

    /* Draw the cone of the spot light */
    engine.polyline.use_color(color);
    engine.polyline.draw_cone(pos, dir, light.beam_angle * 0.5f, cone_length, 32u);

    /* Draw the inner circle of the cone, to show it's blend radius */
    const float cone_inner_radius = cone_length * safe_angle * (1.0f - light.spot_blend);
    const glm::vec3 cone_end = pos + dir * cone_length;
    engine.polyline.draw_world_circle(cone_end, dir, cone_inner_radius, 32u);
}

/* Fading radius for tube light. */
constexpr float TUBE_FADE_RADIUS = 2.0f;

/* Draw tube light overlay. */
inline void tube_light_overlay(Transform& transform, TubeLight light, bool selected) {
    /* Get the position of the light source */
    const glm::vec3 pos = transform.get_world_position();
    const glm::vec3 dir = transform.get_forward();
    const float half_length = transform.get_world_scale().z * 0.5f;
    const glm::vec3 color = selected ? colors::SELECTED : colors::DARK;
    const glm::vec3 camera_pos = engine.renderer.render_view.gpu_view.origin;
    const float camera_dist = glm::distance(camera_pos, pos);

    /* Draw the culling distance */
    const float culling_distance = light.attenuation_distance + half_length;
    if (camera_dist > culling_distance) {
        engine.polyline.use_color(color, (camera_dist - culling_distance) / SPHERE_FADE_RADIUS);
        engine.polyline.draw_circle(pos, culling_distance, 32u); /* Culling distance */
    }

    /* Draw the source tube area */
    const float tube_distance = glm::max(half_length, light.source_radius);
    if (camera_dist > tube_distance) {
        engine.polyline.use_color(color, (camera_dist - tube_distance) / TUBE_FADE_RADIUS);
        engine.polyline.draw_tube(pos - dir * half_length, pos + dir * half_length, light.source_radius, 16u);
    }
}

/* Draw the overlay for light components. */
void draw_lights_overlay() {
    /* Capture all lights in the scene */
    const entt::basic_group group = engine.ecs.group<const Light>(entt::get<Transform>);

    /* Get the camera position is world-space */
    const glm::vec3 camera_pos = engine.renderer.render_view.gpu_view.origin;

    /* 1.5px line width */
    engine.polyline.use_line_width(1.5f);

    /* Iterate over all lights */
    for (auto&& [entity, light, transform] : group.each()) {
        /* Get the position of the light source */
        const glm::vec3 light_pos = transform.get_world_position();
        /* Check if this sphere light is selected */
        const bool selected = editor.windows[Editor::Mode::SCENE].get<Hierarchy>().is_entity_selected(entity);

        /* Draw the reticle */
        engine.polyline.use_color(selected ? colors::SELECTED : colors::DARK);
        const float distance = glm::distance(camera_pos, light_pos);
        engine.polyline.draw_circle(light_pos, distance * OVERLAY_RETICLE_SIZE, 4u);

        switch (light.type) {
            /* Sphere light overlay */
            case LightType::SPHERE_LIGHT:
                sphere_light_overlay(transform, std::get<SphereLight>(light.light), selected);
                break;
            /* Sun light overlay */
            case LightType::SUN_LIGHT:
                sun_light_overlay(transform, selected);
                break;
            /* Spot light overlay */
            case LightType::SPOT_LIGHT:
                spot_light_overlay(transform, std::get<SpotLight>(light.light), selected);
                break;
            /* Tube light overlay */
            case LightType::TUBE_LIGHT:
                tube_light_overlay(transform, std::get<TubeLight>(light.light), selected);
                break;
            default:
                break;
        }
    }
}

}  // namespace tmt
