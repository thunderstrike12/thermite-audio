#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "engine/core/reflection.hpp"
#include "engine/core/resources/envmap.hpp"

namespace tmt {

/* Environment component used to configure the environment lighting. */
struct Environment {
    ResourceRef<Envmap> resource {};

    /* Any objects before this distance will be fully opaque. */
    float object_opaque_distance = 128.0f;
    /* Any objects after this distance will be fully transparent. */
    float object_transparent_distance = 152.0f;
    /* Factor to change the interpolation curve when going from opaque to transparent. (default: 10) */
    float object_opacity_transition = 10.0f;
};

}  // namespace tmt

TMT_COMPONENT(tmt::Environment, "Environment", (resource, object_opaque_distance, object_transparent_distance, object_opacity_transition));
