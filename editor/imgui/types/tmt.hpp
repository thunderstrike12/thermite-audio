#pragma once

#include <ImReflect.hpp>
#include <engine/core/resources.hpp>

template <typename T>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::ResourceRef<T>& value, ImSettings& settings, ImResponse& response) {
    ImGui::BeginGroup();
    ImReflect::Input(label, value.file_location, settings, response);
    if (ImGui::Button("Load")) {
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

    if (ImGui::BeginDragDropTarget()) {
        // Get the current payload to check if its a FileLocation.
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload != nullptr && payload->IsDataType("FileLocation")) {
            const std::string_view json_string {static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize)};
            tmt::IO::FileLocation file_location;
            tmt::Serializer::deserialize(nlohmann::ordered_json::parse(json_string), file_location);

            // Now that we know the user is dragging a FileLocation type, we can check its file extension to see if its valid for the resource type.
            const std::string& extension = file_location.relative_path.extension().generic_string();
            if (T::SUPPORTED_FILE_EXTENSIONS.contains(extension) && ImGui::AcceptDragDropPayload("FileLocation") != nullptr) {
                // If the file extension is valid start excepting the payload, this will return true once the user drops it.
                value.file_location = file_location;

                // TODO: Sven move this if-check to resources.hpp
                if constexpr (std::derived_from<T, tmt::FileResource>) {
                    value.resource = tmt::engine.resources.load_resource<T>(value.file_location).resource;
                } else if constexpr (requires { typename T::is_runtime_resource; }) {
                    value.resource = tmt::engine.resources.copy_resource<T>(value.file_location).resource;
                } else {
                    static_assert(svh::always_false<T>::value, "JsonSerializer Error: Cannot deserialize ResourceRef<T> where T is not a FileResource or RuntimeResource");
                }
            }
        }

        ImGui::EndDragDropTarget();
    }

    if constexpr (ImReflect::Detail::has_imreflect_input_v<T>) {
        ImReflect::Input(label, value.resource, settings, response);
    }
}
