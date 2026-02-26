#pragma once
#include <ImReflect.hpp>
#include "projects/game/components/upgrade.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char* name, game::Upgrade& value, ImSettings& settings, ImResponse& response) {
    using namespace game;

    ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);
    if (ImGui::Button("Apply")) {
        value.apply_upgrade();
    } }