#pragma once

#include <ImReflect.hpp>
#include "engine/core/audio.hpp"

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::AudioEvent& value, ImSettings& settings, ImResponse& response);

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::AudioParameter& value, ImSettings& settings, ImResponse& response);

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::VolumeControl& value, ImSettings& settings, ImResponse& response);
