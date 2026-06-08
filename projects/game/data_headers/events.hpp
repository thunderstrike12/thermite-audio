#pragma once
#include "engine/core/entity.hpp"

#include <glm/glm.hpp>

namespace game {

enum class WeaponType;

struct ShootEvent {
    tmt::Entity shooting_entity;
    bool secondary_shot = false;
    WeaponType type;
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
struct BargeFuelChanged {
    tmt::Entity barge_fuel_entity;
    float new_value;
};

struct TriggerCollisionEvent {
    tmt::Entity trigger;
    tmt::Entity other_object;
};

struct TriggerMovementEvent {
    tmt::Entity trigger;
};
struct TriggerMovementStopEvent {};

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
struct InRangeEvent {
    tmt::Entity entity;
    bool in_range;
};
struct PlayerMaxHealthChanged {
    tmt::Entity player_entity;
    float new_value;
    float previous_value;
};
struct PlayerHealthChanged {
    tmt::Entity player_entity;
    float new_value;
    float previous_value;
};
struct PlayerMaxEnergyChanged {
    tmt::Entity player_entity;
    float new_value;
    float previous_value;
};
struct PlayerEnergyChanged {
    tmt::Entity player_entity;
    float new_value;
    float previous_value;
};
struct EndRun {
    bool player_dead;
};

struct ThermiteExplosionEvent {};

struct GamePausedEvent {};

struct GameUnpausedEvent {};

struct IconTransitionEvent {
    tmt::Entity icon_entity;
    float current_value;
    float min_value;
    float max_value = 1.0f;
};

struct MineVoxelEvent {
    tmt::Entity weapon_entity;
};
struct MineNothingEvent {
    tmt::Entity weapon_entity;
};
struct UpgradesWasPurchasedEvent {
    tmt::Entity purchased_upgrade_entity;
};

}  // namespace game
