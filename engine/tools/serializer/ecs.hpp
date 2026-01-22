#pragma once
#include <set>
#include <JsonReflect.hpp>
#include "engine/core/entity.hpp"

namespace tmt {
class Ecs;
}

/* ECS */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Ecs& ecs);

/* Single entity */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, tmt::Ecs& ecs);

/* Set of entity */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, std::set<tmt::Entity>& entities, tmt::Ecs& ecs);