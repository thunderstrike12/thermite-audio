#pragma once

#include <ImReflect.hpp>

#include "engine/core/components/environment.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Environment& value, ImSettings& settings, ImResponse& response);
