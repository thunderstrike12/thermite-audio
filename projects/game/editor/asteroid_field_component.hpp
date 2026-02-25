#pragma once
#include <ImReflect.hpp>
#include "projects/game/components/asteroid_field_component.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char* name, game::AsteroidFieldComponent& value, ImSettings& settings, ImResponse& response) {
    using namespace game;
    settings.push_member<&FAsteroidLayerEntry::radius_factor>().min(1.f).clamp(true);
    settings.push_member<&FAsteroidLayerEntry::belt_bias>().min(0.f).clamp(true);

    ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);

    auto& type_resp = response.get<AsteroidFieldComponent>();

    static bool reload_toggle = false;
    ImGui::Checkbox("Enable instant reload", &reload_toggle);

    if (!reload_toggle) {
        if (ImGui::Button("Reload field")) {
            value.clear_children();
            value.spawn_field();
        }
    } else {
        if (type_resp.is_changed()) {
            value.clear_children();
            value.spawn_field();
        }
    }
}