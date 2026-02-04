#include "light.hpp"

#include <glm/ext/scalar_constants.hpp>

#include "engine/shared/colorspace.hpp"

namespace tmt {

glm::vec3 Light::calculate_luminance(const glm::vec3 scale) const {
    /* Calculate the linear ACEScg temperture color */
    const glm::vec3 temperature_acescg = cs::black_body_acescg(temperature);
    float luminance = 0.0f;

    switch (type) {
        /* Spherical area light */
        case LightType::SPHERE_LIGHT: {
            const tmt::SphereLight sphere_light = std::get<tmt::SphereLight>(light);
            const float radius = sphere_light.source_radius;
            const float area = 4.0f * glm::pi<float>() * radius * radius;
            luminance = sphere_light.luminous_flux / (glm::pi<float>() * area);
            break;
        }
        /* Sun area light (disk) */
        case LightType::SUN_LIGHT: {
            const tmt::SunLight sun_light = std::get<tmt::SunLight>(light);
            const float radius = glm::tan(sun_light.source_angle);
            const float area = glm::pi<float>() * radius * radius;
            luminance = sun_light.luminous_intensity / area;
            break;
        }
        /* Spot light (disk at aperture) */
        case LightType::SPOT_LIGHT: {
            const tmt::SpotLight spot_light = std::get<tmt::SpotLight>(light);
            const float radius = spot_light.source_radius;
            const float area = glm::pi<float>() * radius * radius;
            luminance = spot_light.luminous_intensity / area;
            break;
        }
        /* Tube area light (cylinder) */
        case LightType::TUBE_LIGHT: {
            const tmt::TubeLight tube_light = std::get<tmt::TubeLight>(light);
            const float length = scale.z;
            const float area = 2.0f * glm::pi<float>() * tube_light.source_radius * length;
            luminance = tube_light.luminous_flux / (glm::pi<float>() * area);
            break;
        }
    }

    return color * temperature_acescg * luminance;
}

}  // namespace tmt
