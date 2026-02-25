#pragma once
#include "engine/core/reflection.hpp"
#include "glm/glm.hpp"
#include "engine/shared/const.hpp"
namespace game {

// TODO use polymorphism interface or similar to support multiple shapes

// TODO had to duplicate this because min and max were not allowing serialization
struct AABB {
    glm::vec3 min_bounds = glm::vec3(-1.0f);
    glm::vec3 max_bounds = glm::vec3(1.0f);
    AABB() = default;
    AABB(const glm::vec3& min_bounds, const glm::vec3& max_bounds) : min_bounds(min_bounds), max_bounds(max_bounds) {}

    bool overlap(const AABB& other) const {
        return (min_bounds.x <= other.max_bounds.x && max_bounds.x >= other.min_bounds.x) && (min_bounds.y <= other.max_bounds.y && max_bounds.y >= other.min_bounds.y) &&
               (min_bounds.z <= other.max_bounds.z && max_bounds.z >= other.min_bounds.z);
    }
};

struct AABBShape {
    AABB aabb;
    float scale = 1.0f;

    AABB world_aabb(const glm::vec3& position) const { return AABB { position + aabb.min_bounds * scale, position + aabb.max_bounds * scale }; }
};

}  // namespace game

TMT_OBJECT(game::AABB, (min_bounds, max_bounds));
TMT_OBJECT(game::AABBShape, (aabb, scale));
