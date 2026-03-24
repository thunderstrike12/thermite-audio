#pragma once

#include "engine/core/resources/json.hpp"
#include "engine/core/components/rig_controller.hpp"

JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::RigController& controller);

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::RigController& controller);