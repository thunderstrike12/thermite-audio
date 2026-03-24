#include "environment.hpp"

#include "editor/imgui/extra.hpp"
#include "editor/imgui/types/resource_ref.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Environment& value, ImSettings& settings, ImResponse& response) {
    ImReflect::Input("Environment Map (HDRI)", value.resource, settings, response);

    ImGui::DragFloat("Object Opaque Distance", &value.object_opaque_distance, 0.1f, 0.0f, 65536.0f, "%.1f u");
    tooltip("Opaque Distance", "Distance before which objects will be completely opaque.");
    if (value.object_transparent_distance < value.object_opaque_distance) value.object_transparent_distance = value.object_opaque_distance;
    ImGui::DragFloat("Object Transparent Distance", &value.object_transparent_distance, 0.1f, value.object_opaque_distance, 65536.0f, "%.1f u");
    tooltip("Transparent Distance", "Distance after which objects will become completely transparent.");
    ImGui::DragFloat("Object Opacity Transition", &value.object_opacity_transition, 0.01f, 10.0f, 100.0f, "%.1f");
    tooltip("Opacity Transition", "Controls the interpolation curve of the opacity from opaque to transparent, higher values make the transition more harsh.");
}
