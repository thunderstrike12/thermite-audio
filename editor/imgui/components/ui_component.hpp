#pragma once
#include <ImReflect.hpp>

#include "engine/core/components/ui_component.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::UIComponent& value, ImSettings& settings, ImResponse& response);