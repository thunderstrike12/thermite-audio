#include "environment.hpp"

#include "editor/imgui/extra.hpp"
#include "editor/imgui/types/resource_ref.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Environment& value, ImSettings& settings, ImResponse& response) {
    ImReflect::Input("Environment Map (HDRI)", value.resource, settings, response);

    ImGui::SeparatorText("Object Culling");
    ImGui::DragFloat("Object Opaque Distance", &value.object_opaque_distance, 0.1f, 0.0f, 65536.0f, "%.1f u");
    tooltip("Opaque Distance", "Distance before which objects will be completely opaque.");
    if (value.object_transparent_distance < value.object_opaque_distance) value.object_transparent_distance = value.object_opaque_distance;
    ImGui::DragFloat("Object Transparent Distance", &value.object_transparent_distance, 0.1f, value.object_opaque_distance, 65536.0f, "%.1f u");
    tooltip("Transparent Distance", "Distance after which objects will become completely transparent.");
    ImGui::DragFloat("Object Opacity Transition", &value.object_opacity_transition, 0.01f, 10.0f, 100.0f, "%.1f");
    tooltip("Opacity Transition", "Controls the interpolation curve of the opacity from opaque to transparent, higher values make the transition more harsh.");

    ImGui::SeparatorText("Fog Options");
    ImGui::DragFloat("Fog Anisotropy", &value.fog_anisotropy, 0.01f, 0.0f, 1.0f, "%.2f");
    tooltip("Anisotropy", "Higher values bias scattering in the forward direction, lower values reduce that bias.");
    ImGui::DragFloat("Fog Scattering Strength", &value.fog_scatter_strength, 0.1f, 0.0f, 1.0f, "%.3f");
    tooltip("Scattering Strength", "Controls the strength of light scattered by fog.");
    ImGui::DragFloat3("Fog Absorption", &value.fog_absorption.x, 0.01f, 0.0f, 10.0f);
    tooltip("Absorption", "Controls the absorption of the fog.");

    ImGui::SeparatorText("Miscellaneous Options");
    ImGui::DragFloat("Particle Reflectance", &value.particle_reflectance, 0.01f, 0.0f, 1.0f, "%.3f");
    tooltip("Reflectance", "Controls what percentage of incoming light is reflected for particles.");
}
