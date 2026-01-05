#pragma once
#include <imgui.h>
#include <ImReflect.hpp>

#include "engine/systems/gameplay/game_component.hpp"

#include "engine/systems/gameplay/game_component_registry.hpp"
#include "engine/core/components/component_collection.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::IGameComponent& value, ImSettings& settings, ImResponse& response) {
    // call inspect
    value.inspect(settings, response);
}

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::ComponentCollection& value, ImSettings& settings, ImResponse& response) {
    const auto& registered_components = tmt::engine.component_registry.get_registered_components();

    for (auto& [component_index, info] : registered_components) {
        if (value.has_component(component_index) == false) {
            continue;
        }
        ImGui::PushID(info.name.c_str());
        if (ImGui::CollapsingHeader(info.name.c_str())) {
            tmt::IGameComponent& component = value.get_component(component_index);
            ImReflect::Input("", component, settings, response);
        }
        ImGui::PopID();
    }
}
