#pragma once
#include "engine/core/entity.hpp"
#include <glm/glm.hpp>

namespace game {

struct ShootEvent {
    tmt::Entity shooting_entity;
};
struct WeaponFiredEvent {
    tmt::Entity weapon_entity;
    glm::vec3 origin;
    glm::vec3 up;
    glm::vec3 direction;
};
struct ProjectileHit {
    tmt::Entity projectile_entity;
    tmt::Entity hit_entity;
};

}  // namespace game
