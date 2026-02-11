#pragma once

#include "engine/core/resources/texture_2d.hpp"

namespace tmt {

struct ParticleEmitter {
    ParticleEmitter();

    glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
    float particle_lifetime = 1.0f;

    glm::vec3 dir = { 0.0f, 1.0f, 0.0f };
    float cone_angle = 0.5f;  // In Degrees

    float min_speed = 1.0f;
    float max_speed = 10.0f;
    uint32_t spawn_count = 8u;

    bool active = true;

    ResourceRef<Texture2D> texture;
};

}  // namespace tmt

TMT_COMPONENT(tmt::ParticleEmitter, "ParticleEmitter", (pos, particle_lifetime, dir, cone_angle, min_speed, max_speed, spawn_count, active, texture));