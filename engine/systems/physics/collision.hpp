#pragma once
#include "core/ecs.hpp"
#include "glm/vec3.hpp"

namespace tmt {
struct ContactPoint {
    glm::vec3 point = {};
    float penetration = 0.0f;
    float tangent_impulse = 0.0f;
    glm::vec3 normal = {};
    float normal_impulse = 0.0f;
    float normal_mass = 0.0f;
    uint32_t contact_index;
};

class Collision {
    uint64_t id = 0;

   public:
    Collision() {};
    Collision(Entity a, Entity b) {
        entity_a = a;
        entity_b = b;
        id = (static_cast<uint64_t>((uint32_t)a) << 32) | (uint32_t)b;
    }

    Entity entity_a = entt::null;
    Entity entity_b = entt::null;
    glm::vec3 normal = {};

    std::vector<ContactPoint> contacts = {};

    inline uint64_t get_id() const { return id; };
};
}  // namespace tmt
