#pragma once
#include <ImReflect.hpp>
#include "projects/game/components/managers/weapon_manager.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char* name, game::WeaponManager::WeaponProcAnimData& value, ImSettings& settings, ImResponse& response) {
    if (ImGui::CollapsingHeader(name)) {
        ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);
    }
}