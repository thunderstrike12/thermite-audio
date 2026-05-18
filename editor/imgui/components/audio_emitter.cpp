#include "audio_emitter.hpp"

#include "engine/core/components/audio_emitter.hpp"
#include "editor/imgui/types/audio.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::AudioEmitter& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::AudioEmitter>();
    auto& type_response = response.get<tmt::AudioEmitter>();

    {
        struct PlayOnStart {};
        auto& play_on_start_settings = type_settings.get<PlayOnStart>();
        auto& play_on_start_response = type_response.get<PlayOnStart>();

        play_on_start_settings.push<bool>();

        ImReflect::Input("Play On Start", value.play_on_start, play_on_start_settings, play_on_start_response);

        if (value.play_on_start) {
            struct Event {};
            auto& event_settings = type_settings.get<Event>();
            auto& event_response = type_response.get<Event>();

            play_on_start_settings.push<tmt::AudioEvent>();

            ImReflect::Input("Audio Event", value.event_on_start, event_settings, event_response);
        }
    }
}