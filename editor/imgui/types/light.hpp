#pragma once

#include <ImReflect.hpp>

#include "engine/core/components/light.hpp"
#include "engine/shared/colorspace.hpp"
#include "editor/font/icon_lookups.hpp"

inline void color_preview() {}

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Light& value, ImSettings&, ImResponse&);
