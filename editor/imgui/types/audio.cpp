#include "audio.hpp"

#include "engine/core/audio.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/audio.hpp"

#include "editor/imgui/extra.hpp"

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::AudioEvent& value, ImSettings& settings, ImResponse& response) {
    ImGui::Text(ICON_MS_PLAYLIST_PLAY);
    tooltip("AudioEvent");
    ImGui::SameLine();

    if (ImGui::Button(ICON_MS_CLEAR)) value = {};
    ImGui::SameLine();

    settings.push<std::string>().read_only().elide_left();
    std::string path = (value.is_valid() ? value.get_path() : "");
    ImReflect::Input(label, path, settings, response);

    if (!ImGui::BeginDragDropTarget()) return;

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AudioEvent");
    if (payload != nullptr) {
        const std::string_view json_string { static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize) };
        tmt::Serializer::deserialize(nlohmann::ordered_json::parse(json_string), value);
    }

    ImGui::EndDragDropTarget();
}

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::AudioParameter& value, ImSettings& settings, ImResponse& response) {
    ImGui::Text(ICON_MS_EDIT_AUDIO);
    tooltip("AudioParameter");
    ImGui::SameLine();

    if (ImGui::Button(ICON_MS_CLEAR)) value = {};
    ImGui::SameLine();

    settings.push<std::string>().read_only().elide_left();
    std::string name = (value.is_valid() ? value.get_name() : "");
    ImReflect::Input(label, name, settings, response);

    if (!ImGui::BeginDragDropTarget()) return;

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AudioParameter");
    if (payload != nullptr) {
        const std::string_view json_string { static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize) };
        tmt::Serializer::deserialize(nlohmann::ordered_json::parse(json_string), value);
    }

    ImGui::EndDragDropTarget();
}

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::VolumeControl& value, ImSettings& settings, ImResponse& response) {
    ImGui::Text(ICON_MS_GRAPHIC_EQ);
    tooltip("VolumeControl");
    ImGui::SameLine();

    if (ImGui::Button(ICON_MS_CLEAR)) value = {};
    ImGui::SameLine();

    settings.push<std::string>().read_only().elide_left();
    std::string path = (value.is_valid() ? value.get_path() : "");
    ImReflect::Input(label, path, settings, response);

    if (!ImGui::BeginDragDropTarget()) return;

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VolumeControl");
    if (payload != nullptr) {
        const std::string_view json_string { static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize) };
        tmt::Serializer::deserialize(nlohmann::ordered_json::parse(json_string), value);
    }

    ImGui::EndDragDropTarget();
}
