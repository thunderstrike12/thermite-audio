#pragma once
#include <ImReflect.hpp>
#include "engine/tools/uuid.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::UUID& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::UUID>();
    auto& type_response = response.get<tmt::UUID>();

    std::string uuid_str = value.str();
    const bool changed = ImGui::InputText(label, &uuid_str);
    if (changed) {
        value.fromStr(uuid_str.c_str());
        type_response.changed();
    }
    ImReflect::Detail::check_input_states(type_response);
}