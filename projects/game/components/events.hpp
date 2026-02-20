#pragma once
#include "engine/core/entity.hpp"
#include <glm/glm.hpp>

namespace game {

struct ShootEvent {
    tmt::Entity shooting_entity;
};
struct WeaponFiredEvent {
    entt::entity weapon_entity;
    glm::vec3 origin;
    glm::vec3 up;
    glm::vec3 direction;
};

}  // namespace game
