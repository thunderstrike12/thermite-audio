import particles;
import math;
import view;

[[vk::push_constant]]
uint32_t emitter_index;

ConstantBuffer<RenderView> render_view;
StructuredBuffer<ParticleEmitter> emitters;
StructuredBuffer<ParticleEffect> effects;

RWStructuredBuffer<Counters> counters;
RWStructuredBuffer<uint32_t> alive_list;
RWStructuredBuffer<uint32_t> alive_list_new;
RWStructuredBuffer<uint32_t> dead_list;
RWStructuredBuffer<Particle> particles;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    ParticleEmitter emitter = emitters[emitter_index];

    if (thread_id.x >= emitter.total_spawn_count)
        return;

    if (emitter.total_spawn_count >= counters[0].dead_count)
        return;

    // Find Which Effect This Thread Belongs To (based on cumulative spawn counts)
    uint32_t effect_index = 0;
    uint32_t offset = 0;
    for (uint32_t i = 0; i < emitter.effects_count; i++) {
        uint32_t spawn_count = effects[emitter.effects_offset + i].spawn_count;
        if (thread_id.x < offset + spawn_count) {
            effect_index = i;
            break;
        }
        offset += spawn_count;
    }

    uint seed = thread_id.x + render_view.frame_index * 1337;

    ParticleEffect effect = effects[emitter.effects_offset + effect_index];

    Particle p;
    p.pos = effect.pos;
    p.age = 0.0f;
    p.lifetime = effect.lifetime;
    p.size = effect.start_size.min;
    p.start_size = lerp(effect.start_size.min, effect.start_size.max, rand_float(seed));
    p.end_size = lerp(effect.end_size.min, effect.end_size.max, rand_float(seed));
    p.opacity = 1.0f;
    p.start_opacity = lerp(effect.start_opacity.min, effect.start_opacity.max, rand_float(seed));
    p.end_opacity = lerp(effect.end_opacity.min, effect.end_opacity.max, rand_float(seed));
    p.rot_speed = lerp(effect.rotation.min, effect.rotation.max, rand_float(seed));
    p.rotation = 0.0f;
    p.pos_jitter = lerp(effect.pos_jitter.min, effect.pos_jitter.max, rand_float(seed)) * effect.jitter_speed;

    p.size_curve = effect.size_curve;
    p.opacity_curve = effect.opacity_curve;

    p.tex_index = effect.tex_index;
    p.flipbook_frames = effect.flipbook_frames;
    p.anim_speed = effect.anim_speed;
    p.current_frame = 0;
    p.dither_scale = effect.dither_scale;

    float3 dir = sample_cone(effect.dir, effect.cone_angle, rand_float2(seed));
    p.start_speed = lerp(effect.start_speed.min, effect.start_speed.max, rand_float(seed));
    p.end_speed = lerp(effect.end_speed.min, effect.end_speed.max, rand_float(seed));

    p.dir = dir;
    p.lifetime = effect.lifetime;
    
    uint32_t count_dead;
    InterlockedAdd<uint32_t>(counters[0].dead_count, -1, count_dead);

    uint32_t new_particle_index = dead_list[count_dead - 1];
    particles[new_particle_index] = p;

    uint32_t alive_count;
    InterlockedAdd<uint32_t>(counters[0].alive_count_after_sim, 1, alive_count);
    alive_list[alive_count] = new_particle_index;
}
