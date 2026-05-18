#pragma once

#include "engine/core/reflection.hpp"
#include "engine/core/resources/lut.hpp"

namespace tmt {

/* Color grading component used to configure the scene color grading. */
struct ColorGrading {
    ResourceRef<LUT> resource {};
};

}  // namespace tmt

TMT_COMPONENT(tmt::ColorGrading, "Color Grading", (resource));