#include "light.hpp"

#include "editor/imgui/extra.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Light& value, ImSettings&, ImResponse&) {
    auto tab_flags = [&](tmt::LightType t) { return (value.type == t) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None; };

    /* List of light type labels */
    const char* LIGHT_TYPE_LABELS[] { "Sphere Light", "Sun Light", "Spot Light", "Tube Light" };
    const uint32_t selected = (uint32_t)magic_enum::enum_index<tmt::LightType>(value.type).value_or(0u);

    /* Light type selection */
    if (ImGui::BeginCombo("Light Type", LIGHT_TYPE_LABELS[selected])) {
        constexpr uint32_t COUNT = sizeof(LIGHT_TYPE_LABELS) / sizeof(char*);
        for (uint32_t i = 0u; i < COUNT; ++i) {
            const bool is_selected = (selected == i);
            if (ImGui::Selectable(LIGHT_TYPE_LABELS[i], is_selected)) {
                value.type = magic_enum::enum_cast<tmt::LightType>(i).value_or(tmt::LightType::SPHERE_LIGHT);
                switch (value.type) {
                    case tmt::LightType::SPHERE_LIGHT:
                        value.light = tmt::SphereLight();
                        break;
                    case tmt::LightType::SUN_LIGHT:
                        value.light = tmt::SunLight();
                        break;
                    case tmt::LightType::SPOT_LIGHT:
                        value.light = tmt::SpotLight();
                        break;
                    case tmt::LightType::TUBE_LIGHT:
                        value.light = tmt::TubeLight();
                        break;
                }
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    /* Light type specific parameters */
    switch (value.type) {
        /* Sphere area light */
        case tmt::LightType::SPHERE_LIGHT: {
            tmt::SphereLight light = std::get<tmt::SphereLight>(value.light);

            ImGui::DragFloat("Source Radius", &light.source_radius, 0.025f, 0.0f, light.attenuation_radius, "%.3f u");
            tooltip("Radius of the sphere light source. (larger means softer shadows)");
            ImGui::DragFloat("Attenuation Radius", &light.attenuation_radius, 0.1f, light.source_radius, 100.0f, "%.3f u");
            tooltip("Radius at which the light influence will be zero.");

            ImGui::DragFloat("Luminous Flux", &light.luminous_flux, 1.0f, 0.0f, 10000.0f, "%.3f lm");
            tooltip("Luminous flux of the sphere light.");

            value.light = light;
            break;
        }
        /* Directional area light */
        case tmt::LightType::SUN_LIGHT: {
            tmt::SunLight light = std::get<tmt::SunLight>(value.light);

            float source_angle = glm::degrees(light.source_angle);
            if (ImGui::DragFloat("Source Angle", &source_angle, 0.1f, 0.0f, 90.0f, "%.3f deg")) {
                light.source_angle = glm::radians(source_angle);
            }
            tooltip("Angle between the center of the sun and its edge, as seen from your position. (larger means softer shadows)");

            ImGui::DragFloat("Luminous Intensity", &light.luminous_intensity, 1.0f, 0.0f, 10000.0f, "%.3f cd");
            tooltip("Luminous intensity of the sun light.");

            value.light = light;
            break;
        }
        /* Spot light */
        case tmt::LightType::SPOT_LIGHT: {
            tmt::SpotLight light = std::get<tmt::SpotLight>(value.light);

            ImGui::DragFloat("Source Radius", &light.source_radius, 0.025f, 0.0f, light.attenuation_distance, "%.3f u");
            tooltip("Radius of the spot light source. (larger means softer shadows)");
            ImGui::DragFloat("Attenuation Distance", &light.attenuation_distance, 0.1f, light.source_radius, 100.0f, "%.3f u");
            tooltip("Distance at which the light influence will be zero.");
            float beam_angle = glm::degrees(light.beam_angle);
            if (ImGui::DragFloat("Beam Angle", &beam_angle, 0.1f, 1.0f, 180.0f, "%.3f deg")) {
                light.beam_angle = glm::radians(beam_angle);
            }
            tooltip("Angular diameter of the spot light beam.");
            float spot_blend = light.spot_blend * 100.0f;
            if (ImGui::DragFloat("Spot Blend", &spot_blend, 0.1f, 0.0f, 100.0f, "%.3f %")) {
                light.spot_blend = spot_blend / 100.0f;
            }
            tooltip("The softness of the spot light edge.");

            ImGui::DragFloat("Luminous Intensity", &light.luminous_intensity, 1.0f, 0.0f, 10000.0f, "%.3f cd");
            tooltip("Luminous intensity of the spot light.");

            value.light = light;
            break;
        }
        /* Tube area light */
        case tmt::LightType::TUBE_LIGHT: {
            tmt::TubeLight light = std::get<tmt::TubeLight>(value.light);

            ImGui::DragFloat("Source Radius", &light.source_radius, 0.025f, 0.0f, light.attenuation_distance, "%.3f u");
            tooltip("Radius of the tube light source. (larger means softer shadows)");
            ImGui::DragFloat("Attenuation Distance", &light.attenuation_distance, 0.1f, light.source_radius, 100.0f, "%.3f u");
            tooltip("Distance at which the light influence will be zero.");

            ImGui::DragFloat("Luminous Flux", &light.luminous_flux, 1.0f, 0.0f, 10000.0f, "%.3f lm");
            tooltip("Luminous flux of the tube light.");

            value.light = light;
            break;
        }
    }

    ImGui::Separator();

    /* Shared parameters */
    ImGui::DragFloat3("Color (ACEScg)", &value.color.x, 0.01f, 0.0f, 1.0f);
    tooltip("Color of the light emitted. (in ACEScg, multiplied together with temperature)");
    ImGui::DragFloat("Temperature", &value.temperature, 10.0f, 1000.0f, 16000.0f, "%.3f K");
    tooltip("Temperature of the light emitted. (in kelvin, default: 6500k whitepoint)");

    /* Calculate the non-linear SRGB (Rec.709) color of the light source */
    const glm::vec3 src = tmt::cs::acescg_to_r709(value.color);
    const glm::vec3 dst = tmt::cs::r709_to_acescg(src);

    const glm::vec3 linear_srgb = tmt::cs::acescg_to_r709(value.color * tmt::cs::black_body_acescg(value.temperature));
    const glm::vec3 nonlinear_srgb = tmt::cs::delinearize(linear_srgb);
    const ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(nonlinear_srgb.x, nonlinear_srgb.y, nonlinear_srgb.z, 1.0f));

    /* Draw a little color preview */
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetCursorScreenPos() + ImVec2(0.0f, 4.0f);
    const ImVec2 max = min + ImVec2(ImGui::GetContentRegionAvail().x, 64.0f);
    draw_list->AddRectFilled(min, max, color, 0.0f);
    ImGui::SetCursorScreenPos(ImVec2(min.x, max.y + 4.0f));
}