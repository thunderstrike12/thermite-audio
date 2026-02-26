#pragma once
#include <engine/tools/types/color.hpp>

inline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::RGBA& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::RGBA>();
    auto& type_response = response.get<tmt::RGBA>();

    bool changed = false;
    changed = ImGui::ColorEdit4(label, &value.get().x, ImGuiColorEditFlags_Float);

    if (changed) {
        type_response.changed();
    }
    ImReflect::Detail::check_input_states(type_response);
}