#pragma once

#include <ImReflect.hpp>
#include <engine/core/resources.hpp>

// Unpack the trivially copyable buffer containing the IO::FileLocation data.
inline tmt::IO::FileLocation buffer_to_file_location(const void* buffer, const size_t size) {
    constexpr size_t SUB_LOCATION_SIZE = sizeof(tmt::IO::Location);

    const tmt::IO::Location sub_location = *static_cast<const tmt::IO::Location*>(buffer);
    const std::string_view relative_path_view {static_cast<const char*>(buffer) + SUB_LOCATION_SIZE, size - SUB_LOCATION_SIZE};

    return {sub_location, relative_path_view};
}

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
            tmt::IO::FileLocation file_location = buffer_to_file_location(payload->Data, payload->DataSize);

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
