#pragma once

#include <ImReflect.hpp>

#include "engine/engine.hpp"
#include "engine/core/components/rig_controller.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, const tmt::RigController& controller, ImSettings&, ImResponse&) {
    ImGui::Text("Parameter count: %llu", controller.parameters.size());
    ImGui::Text("State count: %llu", controller.states.size());
    ImGui::Text("Transition count: %llu", controller.transitions.size());
}