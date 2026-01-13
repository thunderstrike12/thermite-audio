#pragma once

#include "engine/core/resources/json.hpp"

namespace tmt {

class AudioEvent;
class VolumeControl;
class AudioListener;

}  // namespace tmt

/* AudioEvent */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::AudioEvent& event);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::AudioEvent& event);

/* VolumeControl */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::VolumeControl& control);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::VolumeControl& control);

/* AudioListener */
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::AudioListener& audio_listener);