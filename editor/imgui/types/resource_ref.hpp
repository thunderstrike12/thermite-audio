#pragma once

#include <ImReflect.hpp>
#include <engine/core/resources.hpp>
#include "editor/imgui/types/io.hpp"

template <typename T>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::ResourceRef<T>& value, ImSettings& settings, ImResponse& response) {
    ImGui::BeginGroup();
    ImReflect::Input(label, value.file_location, settings, response);

    const bool dropped = response.get<tmt::IO::FileLocation>().is_file_dropped();

    if (ImGui::Button("Load") || dropped) {
        // TODO: Sven move this if-check to resources.hpp
        if constexpr (std::derived_from<T, tmt::FileResource>) {
            value.resource = tmt::engine.resources.load_resource<T>(value.file_location).resource;
        } else if constexpr (requires { typename T::is_runtime_resource; }) {
            value.resource = tmt::engine.resources.copy_resource<T>(value.file_location).resource;
        } else {
            static_assert(svh::always_false<T>::value, "JsonSerializer Error: Cannot deserialize ResourceRef<T> where T is not a FileResource or RuntimeResource");
        }
    }
    ImGui::EndGroup();

    if constexpr (ImReflect::Detail::has_imreflect_input_v<T>) {
        ImReflect::Input(label, value.resource, settings, response);
    }
}
