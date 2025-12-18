#pragma once
#include "engine/tools/serializer.hpp"
#include "engine/core/entity.hpp"

JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity) {
    /* Cast entity to number */
    return static_cast<std::uint32_t>(entity);
}
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity) {
    /* cast json to entity */
    entity = static_cast<tmt::Entity>(j.get<std::uint32_t>());
}