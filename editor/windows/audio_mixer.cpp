#include "audio_mixer.hpp"

#include "engine/engine.hpp"
#include "engine/core/audio.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/audio_bank.hpp"
#include "engine/tools/serializer/audio.hpp"
#include "engine/tools/serializer.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace {

template <typename Type>
void audio_drag_source(const std::string& type_name, const Type& value) {
    if (!ImGui::BeginDragDropSource()) return;

    ImGui::Text("%s: %s", type_name.c_str(), value.get_path().c_str());
    const std::string json_string = tmt::Serializer::serialize(value).dump(4);
    ImGui::SetDragDropPayload(type_name.c_str(), json_string.data(), json_string.size());
    ImGui::EndDragDropSource();
}

}  // namespace

namespace tmt {

void AudioMixer::display() {
    display_menu_bar();

    if (selection.index() == 0 && std::get<std::weak_ptr<AudioBank>>(selection).expired()) invalidate_selection();

    if (!ImGui::BeginTable("MixerTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) return;

    ImGui::TableNextColumn();
    if (ImGui::BeginChild("Banks")) {
        constexpr ImGuiTreeNodeFlags default_flags =
            ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
        constexpr ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf;

        const auto& locations = engine.resources.get_resource_locations<AudioBank>();
        for (const IO::FileLocation& location : locations) {
            const auto& bank = engine.resources.load_resource<AudioBank>(location);

            std::vector<AudioEvent> audio_events = bank->get_audio_events();
            std::vector<VolumeControl> volume_controls = bank->get_volume_controls();
            const std::string bank_path = ICON_MS_INVENTORY_2 " " + bank->get_path();

            const bool bank_is_leaf = audio_events.empty() && volume_controls.empty();
            const bool bank_node_open = ImGui::TreeNodeEx(bank_path.c_str(), default_flags | (bank_is_leaf ? leaf_flags : 0) | get_selection_flags(std::weak_ptr {bank.resource}));
            if (ImGui::IsItemClicked()) selection = bank.resource;
            if (bank_node_open) {
                for (const AudioEvent& event : audio_events) {
                    const std::string event_path = ICON_MS_PLAYLIST_PLAY " " + event.get_path();

                    ImGui::TreeNodeEx(event_path.c_str(), default_flags | leaf_flags | ImGuiTreeNodeFlags_NoTreePushOnOpen | get_selection_flags(event));
                    if (ImGui::IsItemClicked()) selection = event;

                    audio_drag_source("AudioEvent", event);
                }

                for (const VolumeControl& control : volume_controls) {
                    const std::string control_path = ICON_MS_GRAPHIC_EQ " " + control.get_path();

                    ImGui::TreeNodeEx(control_path.c_str(), default_flags | leaf_flags | ImGuiTreeNodeFlags_NoTreePushOnOpen | get_selection_flags(control));
                    if (ImGui::IsItemClicked()) selection = control;

                    audio_drag_source("VolumeControl", control);
                }

                ImGui::TreePop();
            }
        }
    }
    ImGui::EndChild();

    ImGui::TableNextColumn();
    if (ImGui::BeginChild("Info")) {
        switch (selection.index()) {
            case 0: {  // AudioBank selected.
                const auto& bank = std::get<std::weak_ptr<AudioBank>>(selection);

                ImGui::Text("Path: %s", bank.lock()->get_path().c_str());
                break;
            }

            case 1: {  // AudioEvent selected
                const auto& event = std::get<AudioEvent>(selection);
                if (event.is_valid()) {
                    static AudioInstance playing_instance {nullptr};
                    if (ImGui::Button(ICON_MS_PLAY_ARROW " play")) {
                        if (playing_instance.is_valid()) playing_instance.stop();
                        playing_instance = event.play();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_MS_STOP " Stop") && playing_instance.is_valid()) playing_instance.stop();

                    ImGui::Text("Path: %s", event.get_path().c_str());

                    ImGui::NewLine();

                    const std::vector<AudioParameter> parameters = event.get_parameters();
                    if (!parameters.empty()) ImGui::Text("Parameters:");

                    ImGui::Indent();
                    for (const AudioParameter& parameter : parameters) {
                        ImGui::Text("Name: %s", parameter.get_name().c_str());
                        ImGui::Text("Range: %.1f-%.1f", parameter.get_min(), parameter.get_max());
                        ImGui::NewLine();
                    }
                    ImGui::Unindent();
                }
                break;
            }

            case 2: {  // VolumeControl selected.
                const auto& control = std::get<VolumeControl>(selection);
                if (control.is_valid()) {
                    ImGui::Text("Path: %s", control.get_path().c_str());

                    ImGui::NewLine();

                    float volume = control.get_volume();
                    if (ImGui::SliderFloat("Volume: ", &volume, 0.0f, 2.0f, "%.2f")) {
                        control.set_volume(volume);
                    }
                }
                break;
            }

            default:
                break;
        }
    }
    ImGui::EndChild();

    ImGui::EndTable();
}

void AudioMixer::on_game_end() {
    cached_banks.clear();
    selection = {};
}

void AudioMixer::display_menu_bar() {
    if (!ImGui::BeginMenuBar()) return;

    static IO::FileLocation resource_location {};
    const bool is_location_valid = !resource_location.relative_path.empty();

    ImGui::BeginDisabled(!is_location_valid);
    if (ImGui::MenuItem("Load Bank:")) {
        const auto bank = engine.resources.load_resource<AudioBank>(resource_location);
        if (std::ranges::find(cached_banks, bank) == cached_banks.end()) cached_banks.push_back(bank);
        resource_location = {};
    }
    ImGui::EndDisabled();

    // Always keep this disabled to avoid the user editing it.
    ImGui::BeginDisabled(resource_location.relative_path.empty());
    std::string resource_text = (is_location_valid ? fmt::format("{}", resource_location) : "");
    ImGui::InputText("##ResourceLocation", &resource_text);
    ImGui::EndDisabled();

    if (ImGui::BeginDragDropTarget()) {
        // Get the current payload to check if it's a FileLocation.
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload != nullptr && payload->IsDataType("FileLocation")) {
            const std::string_view json_string {static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize)};
            IO::FileLocation file_location;
            Serializer::deserialize(nlohmann::ordered_json::parse(json_string), file_location);

            // Now that we know the user is dragging a FileLocation type, we can check its file extension to see if its valid for the resource type.
            const std::string& extension = file_location.relative_path.extension().generic_string();
            if (tmt::AudioBank::SUPPORTED_FILE_EXTENSIONS.contains(extension) && ImGui::AcceptDragDropPayload("FileLocation") != nullptr) {
                // If the file extension is valid start excepting the payload, this will return true once the user drops it.
                resource_location = file_location;
            }
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::EndMenuBar();
}

template <>
int AudioMixer::get_selection_flags<std::weak_ptr<AudioBank>>(const std::weak_ptr<AudioBank>& selection_compare) {
    const auto* value_pointer = std::get_if<std::weak_ptr<AudioBank>>(&selection);
    if (value_pointer == nullptr || value_pointer->lock() != selection_compare.lock()) return 0;

    return ImGuiTreeNodeFlags_Bullet;
}

template <typename Type>
int AudioMixer::get_selection_flags(const Type& selection_compare) {
    const Type* value_pointer = std::get_if<Type>(&selection);
    if (value_pointer == nullptr || (*value_pointer) != selection_compare) return 0;

    return ImGuiTreeNodeFlags_Bullet;
}

void AudioMixer::invalidate_selection() { selection = AudioEvent {}; }
}  // namespace tmt