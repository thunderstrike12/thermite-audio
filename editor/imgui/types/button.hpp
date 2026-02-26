#pragma once
#include <ImReflect.hpp>
#include "engine/core/components/button.hpp"
#include "extern/magic_enum/magic_enum.hpp"
#include "engine/tools/types/direction.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Button& button, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::Button>();
    auto& type_response = response.get<tmt::Button>();

    ImGui::SeparatorText("Button Colors");
    for (size_t i = 0; i < button.colors.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        const auto enum_value = static_cast<tmt::ButtonState>(i);
        const auto enum_label = std::string(magic_enum::enum_name(enum_value));
        ImReflect::Input(enum_label.c_str(), button.colors[i], type_settings, type_response);
        ImGui::PopID();
    }

    ImGui::SeparatorText("Button Flow");
    for (size_t i = 0; i < button.flow_direction.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        const auto enum_value = static_cast<tmt::Direction>(i);
        const auto enum_label = std::string(magic_enum::enum_name(enum_value));
        ImReflect::Input(enum_label.c_str(), button.flow_direction[i], type_settings, type_response);
        ImGui::PopID();
    }
    ImReflect::Detail::check_input_states(type_response);
}
