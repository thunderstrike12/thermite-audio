#pragma once
#include "engine/core/entity.hpp"
#include <glm/glm.hpp>

namespace game {

struct ShootEvent {
    tmt::Entity shooting_entity;
    bool secondary_shot = false;
};
struct ReleaseShootEvent {
    tmt::Entity shooting_entity;
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

struct TriggerMovementEvent {
    tmt::Entity trigger;
};
struct MovementUpdateEvent {
    tmt::Entity moved_entity;
    float length;
};
struct AttachAttemptEvent {
    tmt::Entity entity;
};
struct AttachEvent {
    tmt::Entity entity;
    bool is_attached;
};

}  // namespace game
