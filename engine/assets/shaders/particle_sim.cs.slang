import view;
import particles;
import math;

ConstantBuffer<RenderView> render_view;

StructuredBuffer<ParticleEffect> effects;
RWStructuredBuffer<Particle> particles;
RWStructuredBuffer<uint32_t> alive_list;
RWStructuredBuffer<uint32_t> alive_list_new;
RWStructuredBuffer<uint32_t> dead_list;
RWStructuredBuffer<Counters> counters;

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    uint32_t alive_counter = counters[0].alive_count;
    if (thread_id.x >= alive_counter)
        return;

    uint32_t particle_index = alive_list[thread_id.x];
    Particle p = particles[particle_index];
    uint32_t v0 = particle_index * 6;

    p.age += render_view.dt;

    if (p.age < p.lifetime) {
        p.pos += p.velocity * render_view.dt;

        const float t = p.age / p.lifetime;

        ParticleEffect effect = effects[p.effect_index];

        p.size = lerp(p.start_size, p.end_size, effect.size_curve.evaluate(t));
        p.opacity = lerp(p.start_opacity, p.end_opacity, effect.opacity_curve.evaluate(t));
        p.rotation += p.rot_speed * render_view.dt;

        uint32_t seed = particle_index + render_view.frame_index * 1337;
        float3 new_jitter = (float3(rand_float(seed), rand_float(seed), rand_float(seed)) * 2.0f - 1.0f) * p.pos_jitter;
        p.pos += new_jitter * render_view.dt;

        particles[particle_index] = p;

        uint32_t new_alive_index;
        InterlockedAdd<uint32_t>(counters[0].alive_count_after_sim, 1, new_alive_index);
        alive_list_new[new_alive_index] = particle_index;

        InterlockedAdd(counters[0].draw_count, 1);
    }
    else {
        uint32_t dead_index;
        InterlockedAdd<uint32_t>(counters[0].dead_count, 1, dead_index);
        dead_list[dead_index] = particle_index;
    }
}
