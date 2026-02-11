#pragma once
#include <set>
#include <optional>
#include <JsonReflect.hpp>
#include "engine/core/entity.hpp"
#include "engine/tools/json.hpp"
#include "engine/tools/types/tree_map.hpp"
#include "engine/core/components/prefab.hpp" /* for PrefabInstanceID */

namespace tmt {

class Ecs;

}  // namespace tmt

/* ECS */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data = std::nullopt);

/* Single entity */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data = std::nullopt);

/* Set of entity */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, std::set<tmt::Entity>& entities, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data = std::nullopt);

/* forward declare */
namespace tmt {

struct SerializeState;
struct DeserializeState;

}  // namespace tmt

/* Single entity with states */
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, tmt::SerializeState& state);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, const tmt::DeserializeState& state);
