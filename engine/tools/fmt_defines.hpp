#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/tools/fmt_helpers.hpp"

#include "engine/core/ecs.hpp"

FMT_LOGGING(glm::vec2, "(x={}, y={})", obj.x, obj.y);
FMT_LOGGING(glm::vec3, "(x={}, y={}, z={})", obj.x, obj.y, obj.z);
FMT_LOGGING(glm::vec4, "(x={}, y={}, z={}, w={})", obj.x, obj.y, obj.z, obj.w);
FMT_LOGGING(glm::ivec2, "(x={}, y={})", obj.x, obj.y);
FMT_LOGGING(glm::ivec3, "(x={}, y={}, z={})", obj.x, obj.y, obj.z);
FMT_LOGGING(glm::ivec4, "(x={}, y={}, z={}, w={})", obj.x, obj.y, obj.z, obj.w);
FMT_LOGGING(glm::quat, "(w={}, x={}, y={}, z={})", obj.w, obj.x, obj.y, obj.z);

FMT_LOGGING(tmt::Entity, "{}", static_cast<uint32_t>(obj));