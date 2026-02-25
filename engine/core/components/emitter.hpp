#pragma once

#include "engine/core/resources/texture_2d.hpp"

#include "engine/tools/types/bezier_curve.hpp"

namespace tmt {

struct Range {
    float min;
    float max;
};

struct ParticleEffect {
    ParticleEffect();

    float particle_lifetime = 1.0f;

    glm::vec3 dir = { 0.0f, 1.0f, 0.0f };
    float cone_angle = 10.0f;  // In Degrees

    uint32_t spawn_count = 8u;
    Range speed { 1.0f, 10.0f };

    Range start_size = { 64.0f, 64.0f };
    Range end_size = { 64.0f, 64.0f };
    BezierCurve size_curve {};

    Range start_opacity { 1.0f, 1.0f };
    Range end_opacity { 1.0f, 1.0f };
    BezierCurve opacity_curve {};

    Range rotation { 0.0f, 0.0f };
    Range pos_jitter { 0.0f, 0.0f };
    float jitter_speed = 1.0f;

    bool active = true;

    ResourceRef<Texture2D> texture;
};

struct ParticleEmitter {
    std::vector<ParticleEffect> effects;

    bool active = true;
};

}  // namespace tmt

TMT_OBJECT(tmt::Range, (min, max));
TMT_OBJECT(
    tmt::ParticleEffect,
    (particle_lifetime, dir, cone_angle, spawn_count, speed, start_size, end_size, size_curve, start_opacity, end_opacity, opacity_curve, rotation, pos_jitter, jitter_speed, active, texture)
);

TMT_COMPONENT(tmt::ParticleEmitter, "ParticleEmitter", (effects, active));