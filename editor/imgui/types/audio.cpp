#include "audio.hpp"

#include "engine/core/audio.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/audio.hpp"

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::AudioEvent& value, ImSettings& settings, ImResponse& response) {
    if (ImGui::Button(ICON_MS_CLEAR)) value = {};
    ImGui::SameLine();

    ImGui::BeginDisabled();
    std::string path = (value.is_valid() ? value.get_path() : "");
    ImReflect::Input(label, path, settings, response);
    ImGui::EndDisabled();

    if (!ImGui::BeginDragDropTarget()) return;

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AudioEvent");
    if (payload != nullptr) {
        const std::string_view json_string { static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize) };
        tmt::Serializer::deserialize(nlohmann::ordered_json::parse(json_string), value);
    }

    ImGui::EndDragDropTarget();
}

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::VolumeControl& value, ImSettings& settings, ImResponse& response) {
    if (ImGui::Button(ICON_MS_CLEAR)) value = {};
    ImGui::SameLine();

    ImGui::BeginDisabled();
    std::string path = (value.is_valid() ? value.get_path() : "");
    ImReflect::Input(label, path, settings, response);
    ImGui::EndDisabled();

    if (!ImGui::BeginDragDropTarget()) return;

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VolumeControl");
    if (payload != nullptr) {
        const std::string_view json_string { static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize) };
        tmt::Serializer::deserialize(nlohmann::ordered_json::parse(json_string), value);
    }

    ImGui::EndDragDropTarget();
}