import particles;
import math;
import view;

[[vk::push_constant]]
uint32_t emitter_index;

ConstantBuffer<RenderView> render_view;
StructuredBuffer<ParticleEmitter> emitters;

RWStructuredBuffer<Counters> counters;
RWStructuredBuffer<uint32_t> alive_list;
RWStructuredBuffer<uint32_t> alive_list_new;
RWStructuredBuffer<uint32_t> dead_list;
RWStructuredBuffer<Particle> particles;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    ParticleEmitter emitter = emitters[emitter_index];

    if (thread_id.x >= emitter.spawn_count)
        return;

    if (emitter.spawn_count >= counters[0].dead_count)
        return;

    Particle p;
    p.pos = emitter.pos;
    p.age = 0.0f;
    p.lifetime = emitter.lifetime;
    p.tex_index = emitter.tex_index;

    uint seed = thread_id.x + render_view.frame_index * 1337;

    float3 dir = sample_cone(emitter.dir, radians(emitter.cone_angle), rand_float2(seed));
    float speed = lerp(emitter.min_speed, emitter.max_speed, rand_float(seed));

    p.velocity = dir * speed;
    p.lifetime = emitter.lifetime;

    uint32_t count_dead;
    InterlockedAdd<uint32_t>(counters[0].dead_count, -1, count_dead);

    uint32_t new_particle_index = dead_list[count_dead - 1];

    particles[new_particle_index] = p;

    uint32_t alive_count;
    InterlockedAdd<uint32_t>(counters[0].alive_count_after_sim, 1, alive_count);
    alive_list[alive_count] = new_particle_index;
}
