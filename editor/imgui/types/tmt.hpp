#pragma once

#include <ImReflect.hpp>
#include <engine/core/resources.hpp>

template <typename T>
void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::ResourceRef<T>& value, ImSettings& settings, ImResponse& response) {
    ImReflect::Input(label, value.file_location, settings, response);
    if constexpr (ImReflect::Detail::has_imreflect_input_v<T>) {
        ImReflect::Input(label, value.resource, settings, response);
    }
}
