#pragma once
#include "engine/core/entity.hpp"
#include <glm/glm.hpp>

namespace game {

struct ShootEvent {
    tmt::Entity shooting_entity;
    bool secondary_shot = false;
};
struct WeaponFiredEvent {
    tmt::Entity weapon_entity;
    bool secondary_shot = false;
    glm::vec3 origin;
    glm::vec3 up;
    glm::vec3 direction;
};
struct ProjectileHitEvent {
    tmt::Entity projectile_entity;
    tmt::Entity hit_entity;
};

struct TriggerCollisionEvent {
    tmt::Entity trigger;
    tmt::Entity other_object;
};

}  // namespace game
