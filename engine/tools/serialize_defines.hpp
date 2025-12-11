#pragma once

#include <engine/tools/serializer.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

JSON_REFLECT(glm::vec2, x, y);
JSON_REFLECT(glm::vec3, x, y, z);
JSON_REFLECT(glm::vec4, x, y, z, w);
JSON_REFLECT(glm::quat, w, x, y, z);
JSON_REFLECT(glm::ivec2, x, y);
JSON_REFLECT(glm::ivec3, x, y, z);
JSON_REFLECT(glm::ivec4, x, y, z, w);
JSON_REFLECT(glm::uvec2, x, y);
JSON_REFLECT(glm::uvec3, x, y, z);
JSON_REFLECT(glm::uvec4, x, y, z, w);

#include "engine/core/ecs.hpp"

JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity) {
    /* Cast entity to number */
    return static_cast<std::uint32_t>(entity);
}
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity) {
    /* cast json to entity */
    entity = static_cast<tmt::Entity>(j.get<std::uint32_t>());
}