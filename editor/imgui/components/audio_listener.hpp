#pragma once
#include <ImReflect.hpp>

#include "engine/core/components/audio_listener.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::AudioListener& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::AudioListener>();
    auto& type_response = response.get<tmt::AudioListener>();

    struct Weight {};
    auto& weight_settings = type_settings.get<Weight>();
    auto& weight_response = type_response.get<Weight>();

    weight_settings.push<float>().as_slider().speed(0.1f).min(0.0f).max(1.0f);

    float weight = value.get_weight();
    ImReflect::Input("Weight", weight, weight_settings, weight_response);

    if (weight_response.is_changed()) value.set_weight(weight);
}