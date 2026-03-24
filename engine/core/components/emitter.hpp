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

    glm::vec3 pos_offset = { 0.0f, 0.0f, 0.0f };
    glm::vec3 dir = { 0.0f, 90.0f, 0.0f };  // In Degrees
    float cone_angle = 10.0f;               // In Degrees

    uint32_t spawn_count = 8u;
    float spawn_interval = 0.0f;            // seconds between spawns (0.0f = every frame)

    Range start_speed { 1.0f, 1.0f };
    Range end_speed { 1.0f, 1.0f };
    BezierCurve speed_curve {};

    Range start_size = { 64.0f, 64.0f };
    Range end_size = { 64.0f, 64.0f };
    BezierCurve size_curve {};

    Range start_opacity { 1.0f, 1.0f };
    Range end_opacity { 1.0f, 1.0f };
    BezierCurve opacity_curve {};

    Range rotation_speed { 0.0f, 0.0f };

    Range pos_jitter { 0.0f, 0.0f };
    float jitter_speed = 1.0f;

    float anim_speed = 1.0f;
    float dither_scale = 1.0f;

    bool active = true;
    bool should_burst = false;

    ResourceRef<Texture2D> texture;

    /* Internal Usage */
    std::string name = "Particle Effect";
    glm::vec3 name_color = { 1.0f, 1.0f, 1.0f };
    float spawn_timer = 0.0f;
};

struct ParticleEmitter {
    std::vector<ParticleEffect> effects;

    bool active = true;
    bool should_burst = false;
};

}  // namespace tmt

TMT_OBJECT(tmt::Range, (min, max));
TMT_OBJECT(
    tmt::ParticleEffect, (particle_lifetime, pos_offset, dir, cone_angle, spawn_count, spawn_interval, start_speed, end_speed, speed_curve, start_size, end_size, size_curve, start_opacity,
                          end_opacity, opacity_curve, rotation_speed, pos_jitter, jitter_speed, active, texture, name, name_color)
);

TMT_COMPONENT(tmt::ParticleEmitter, "ParticleEmitter", (effects, active));