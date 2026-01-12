#pragma once
#include <JsonReflect.hpp>

namespace tmt {
/* Forward declare */
class IGameComponent;
class ComponentCollection;
}  // namespace tmt

JsonReflect::json tag_invoke(JsonReflect::serialize_lib_t, const tmt::IGameComponent& value);

void tag_invoke(JsonReflect::deserialize_lib_t, const JsonReflect::json& j, tmt::IGameComponent& value);

JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::ComponentCollection& collection);

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::ComponentCollection& collection);