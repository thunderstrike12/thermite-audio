/**
 * @name Volumetric Lighting [1 thread per pixel]
 * Integrates volumetric lighting from the froxel volume.
 */
import view;
import math;
import volume;
import visibility;
import voxel;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
Texture2D<float4> blue_noise;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
StructuredBuffer<VoxelObject> objects;
/* Froxel buffer. */
Texture3D<float3> froxel_scatter;
/* Linear sampler. */
SamplerState froxel_sampler;
/* Luminance buffer. */
RWTexture2D<float3> lbuffer;

struct Pc {
    float3 fog_absorb; /* default: 0.02 */
    float base_step_size; /* default: 0.1 */
    uint step_count; /* default: 32 */
};
[[vk::push_constant]] Pc p;

/* Transmittance function. */
[ForceInline] float3 beer_lambert(float3 sigma_t, float distance) {
    return exp(-sigma_t * distance);
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Load pixel depth to correctly set maximum ray march depth */
    const uint64_t packed = asuint64(vbuffer[thread_id]);
    float max_t = iv::FROXEL_FAR_Z;
    if (null_visibility(packed) == false) {
        const VisData vis = unpack_visibility(packed);
        const GeoData geo = unpack_geometry(vis, objects, ray);
        max_t = min(iv::FROXEL_FAR_Z, geo.depth);
    }

    /* Jitter the base step size to improve quality */
    const float jitter = r1(blue_noise[thread_id % 512].r, render_view.frame_index % 1024u);
    float step_size = p.base_step_size * jitter;

    /* Ray march the froxel volume */
    float3 inscattered = 0.0f;
    float3 transmittance = 1.0f;
    float t = iv::FROXEL_NEAR_Z;
    for (int i = 0; i < p.step_count; ++i) {
        if (t >= max_t) break; /* Avoid stepping beyond the scene depth */

        /* Calculate UVW coordinates at each step */
        const float3 uvw = iv::froxel_uvw(ray.origin + ray.dir * t, render_view.world_to_clip, iv::FROXEL_NEAR_Z, iv::FROXEL_FAR_Z);

        /* Calculate transmittance and inscattering at each step */
        const float3 step_transmittance = beer_lambert(p.fog_absorb, step_size);
        inscattered += transmittance * froxel_scatter.SampleLevel(froxel_sampler, uvw, 0) * (1.0f - step_transmittance);
        transmittance *= step_transmittance;

        /* Step forward */
        t += step_size;
        step_size *= 2.0f;
    }

    /* Apply inscattering and transmittance */
    if (null_visibility(packed)) {
        lbuffer[thread_id] = lbuffer[thread_id] + inscattered;
    } else {
        lbuffer[thread_id] = lbuffer[thread_id] * transmittance + inscattered;
    }
}
