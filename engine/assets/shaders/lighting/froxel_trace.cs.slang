/**
 * @name Trace Froxel Volume [1 thread per froxel]
 * For each froxel trace a ray to evaluate direct scattering.
 */
import view;
import math;
import volume;
import voxel;
import bvh2;

/* Active render & scene view. */
ConstantBuffer<RenderView> render_view;
ConstantBuffer<SceneView> scene_view;
/* Voxel objects buffers. */
StructuredBuffer<float4> nodes;
StructuredBuffer<uint> indices;
StructuredBuffer<VoxelObject> objects;
/* Froxel buffers. */
Texture3D<float3> prev_froxel_scatter;
RWTexture3D<float3> froxel_scatter;
Texture3D<float3> prev_froxel_luminance;
RWTexture3D<float3> froxel_luminance;
/* Linear sampler. */
SamplerState linear_sampler;

struct Pc {
    float fog_anisotropy; /* default: 0.6 */
    float scatter_strength; /* default: 0.0025 */
    float particle_reflectance; /* default: 0.1 */
};
[[vk::push_constant]] Pc p;

/* Compute a unique seed for a given thread and frame. */
[ForceInline] uint thread_seed(int3 thread_id, uint frame) {
    return thread_id.x + thread_id.y * iv::FROXEL_GRID_SIZE.x + thread_id.z * iv::FROXEL_GRID_SIZE.x * iv::FROXEL_GRID_SIZE.y + render_view.frame_index * iv::FROXEL_GRID_SIZE.x * iv::FROXEL_GRID_SIZE.y * iv::FROXEL_GRID_SIZE.z;
}

/* Anisotropic scattering phase function. */
[ForceInline] float henyey_greenstein(float cos_theta, float g) {
    const float g2 = g * g;
    const float denom = 1.0f + g2 - 2.0f * g * cos_theta;
    return (1.0f - g2) / (4.0f * PI_F32 * pow(denom, 1.5f));
}

[shader("compute")]
[numthreads(4, 4, 4)]
void main(int3 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= iv::FROXEL_GRID_SIZE)) return;

    /* Generate a unique seed for this thread */
    uint seed = thread_seed(thread_id, render_view.frame_index);
    /* Jitter the froxel ID using the unique seed */
    const float3 froxel_id = float3(thread_id) + rand_float3(seed);

    /* Trace occlusion for the sun */
    const float3 froxel_pos = iv::froxel_to_world(froxel_id, render_view.clip_to_world, iv::FROXEL_NEAR_Z, iv::FROXEL_FAR_Z);
    const float t = occlusion_bvh2(nodes, indices, objects, custom_ray(froxel_pos, scene_view.sun_dir), BIG_F32);

    /* Calculate scattering */
    const float phase = henyey_greenstein(dot(normalize(froxel_pos - render_view.origin.xyz), scene_view.sun_dir), frac(p.fog_anisotropy));
    const float3 luminance = select(t < BIG_F32, 0.0f, scene_view.sun_luminance);

    /* Try to re-project history */
    const float3 froxel_center = iv::froxel_to_world(float3(thread_id) + 0.5f, render_view.clip_to_world, iv::FROXEL_NEAR_Z, iv::FROXEL_FAR_Z);
    const float3 history_uvw = iv::froxel_uvw(froxel_center, render_view.prev_world_to_clip, iv::FROXEL_NEAR_Z, iv::FROXEL_FAR_Z);
    if (any(history_uvw < 0.0f) || any(history_uvw >= 1.0f)) {
        froxel_scatter[thread_id] = luminance * phase * p.scatter_strength;
        froxel_luminance[thread_id] = luminance;
        return;
    }

    /* Blend with history */
    const float3 scatter_history = prev_froxel_scatter.SampleLevel(linear_sampler, history_uvw, 0);
    froxel_scatter[thread_id] = lerp(scatter_history, luminance * phase * p.scatter_strength, 0.1f);
    const float3 luminance_history = prev_froxel_luminance.SampleLevel(linear_sampler, history_uvw, 0);
    froxel_luminance[thread_id] = lerp(luminance_history, luminance * p.particle_reflectance, 0.1f);
}
