#include "audio.hpp"

#include "resources.hpp"
#include "engine.hpp"
#include "engine/core/audio.hpp"
#include "engine/core/resources/audio_bank.hpp"
#include "engine/core/components/audio_listener.hpp"
#include "engine/core/components/audio_emitter.hpp"
#include "engine/tools/serializer.hpp"

JSON_REFLECT(FMOD_GUID, Data1, Data2, Data3, Data4);

JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::AudioEvent& event) {
    JsonReflect::json json;

    const tmt::ResourceRef<tmt::AudioBank>& bank = event.get_source_bank();
    if (bank) json["source_bank"] = tmt::Serializer::serialize(bank);

    if (event.is_valid()) json["guid"] = tmt::Serializer::serialize(event.get_guid());

    return json;
}

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::AudioEvent& event) {
    tmt::ResourceRef<tmt::AudioBank> bank {};
    if (j.contains("source_bank")) tmt::Serializer::deserialize(j["source_bank"], bank);

    FMOD_GUID guid {};
    if (j.contains("guid")) tmt::Serializer::deserialize(j["guid"], guid);

    if (bank) event = tmt::AudioEvent {bank, tmt::engine.audio.get_event_description(guid)};
}

JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::VolumeControl& control) {
    JsonReflect::json json;

    const tmt::ResourceRef<tmt::AudioBank>& bank = control.get_source_bank();
    if (bank) json["source_bank"] = tmt::Serializer::serialize(bank);

    if (control.is_valid()) json["guid"] = tmt::Serializer::serialize(control.get_guid());

    return json;
}

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::VolumeControl& control) {
    tmt::ResourceRef<tmt::AudioBank> bank {};
    if (j.contains("source_bank")) tmt::Serializer::deserialize(j["source_bank"], bank);

    FMOD_GUID guid {};
    if (j.contains("guid")) tmt::Serializer::deserialize(j["guid"], guid);

    if (bank) control = tmt::VolumeControl {bank, tmt::engine.audio.get_vca(guid)};
}

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::AudioListener& audio_listener) {
    float weight = 1.0f;
    if (j.contains("cached_weight")) tmt::Serializer::deserialize(j["cached_weight"], weight);

    audio_listener.set_weight(weight);
}