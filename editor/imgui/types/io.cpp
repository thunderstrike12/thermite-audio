#include "io.hpp"
#include "editor/imgui/types/std.hpp"

void tag_invoke(ImReflect::ImInput_t, const char* name, tmt::IO::FileLocation& value, ImSettings& settings, ImResponse& response) {
    auto& location_response = response.get<tmt::IO::FileLocation>();

    ImGui::BeginGroup();
    ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);
    ImGui::EndGroup();

    if (ImGui::BeginDragDropTarget()) {
        // Get the current payload to check if its a FileLocation.
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload != nullptr && payload->IsDataType("FileLocation")) {
            const std::string_view json_string { static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize) };
            tmt::IO::FileLocation file_location;
            tmt::Serializer::deserialize(nlohmann::ordered_json::parse(json_string), file_location);

            if (ImGui::AcceptDragDropPayload("FileLocation") != nullptr) {
                value = file_location;
                location_response.dropped();
                location_response.changed();
            }
        }

        ImGui::EndDragDropTarget();
    }
}