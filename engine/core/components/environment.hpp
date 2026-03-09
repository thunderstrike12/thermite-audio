#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "engine/core/reflection.hpp"
#include "engine/core/resources/envmap.hpp"

namespace tmt {

/* Environment component used to configure the environment lighting. */
struct Environment {
    ResourceRef<Envmap> resource {};
};

}  // namespace tmt

TMT_COMPONENT(tmt::Environment, "Environment", (resource));
