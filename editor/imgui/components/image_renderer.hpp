#pragma once
#include <ImReflect.hpp>

#include "engine/core/components/image_renderer.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::ImageRenderer& value, ImSettings& settings, ImResponse& response);