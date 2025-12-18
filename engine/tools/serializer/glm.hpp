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