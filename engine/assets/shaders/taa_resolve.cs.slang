import colorspace;
import view;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Samplers. */
SamplerState point_sampler;
SamplerState linear_sampler;
/* Motion Vectors buffer. */
Texture2D<float2> mbuffer;
/* Depth buffer. */
Texture2D<float> dbuffer;
/* Luminance buffer. */
RWTexture2D<float3> lbuffer;
/* History buffers. */
RWTexture2D<float4> hbuffer1;
Texture2D<float4> hbuffer2;

[[vk::push_constant]]
uint32_t taa_flag;

float3 clip_aabb(float3 aabb_min, float3 aabb_max, float3 history) {
    const float3 center = 0.5f * (aabb_max + aabb_min);
    const float3 extents = 0.5f * (aabb_max - aabb_min) + 1e-5f;

    const float3 offset = history - center;
    const float3 ts = abs(extents / offset);
    const float t = saturate(min(ts.x, min(ts.y, ts.z)));

    return center + offset * t;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    float3 curr_luminance = lbuffer[thread_id];

    if (taa_flag != 1) {
        lbuffer[thread_id] = curr_luminance;
        return;
    }

    float3 output = float3(0.0f, 0.0f, 0.0f);

    /* Neighborhood clamp to prevent ghosting */
    /* m1 & m2 are used for variance clipping */
    float3 m1 = float3(0.0f, 0.0f, 0.0f);
    float3 m2 = float3(0.0f, 0.0f, 0.0f);
    float3 neighbour_min = curr_luminance;
    float3 neighbour_max = curr_luminance;
    int2 closest_offset = int2(0, 0);
    float closest_depth = 0.0f;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            const float3 s = lbuffer[thread_id + int2(x, y)];
            m1 += s;
            m2 += s * s;
            neighbour_min = min(neighbour_min, s);
            neighbour_max = max(neighbour_max, s);

            const float d = dbuffer[thread_id + int2(x, y)];
            if (d > closest_depth) {
                closest_depth = d;
                closest_offset = int2(x, y);
            }
        }
    }

    /* Variance clipping (build tighter aabb) */
    /* https://alextardif.com/TAA.html */
    const float3 mu = m1 / 9.0f;
    const float3 sigma = sqrt(abs(m2 / 9.0f - mu * mu));
    const float gamma = 1.0f;
    const float3 var_min = max(mu - gamma * sigma, neighbour_min);
    const float3 var_max = min(mu + gamma * sigma, neighbour_max);

    /* Reproject */
    const float2 uv = (float2(thread_id) + 0.5f) / float2(render_view.resolution);
    const float2 uv_last = uv + mbuffer[thread_id + closest_offset];

    /* Sample and clip history */
    float3 prev_luminance = hbuffer2.SampleLevel(linear_sampler, uv_last, 0.0f).rgb;
    prev_luminance = clip_aabb(var_min, var_max, prev_luminance);

    /* Reject off-screen */
    const bool valid = all(uv_last >= 0.0f) && all(uv_last <= 1.0f);
    const float blend = valid ? 0.95f : 0.0f;
            
    output = lerp(curr_luminance, prev_luminance, blend);

    hbuffer1[thread_id] = float4(output, 1.0f);
    lbuffer[thread_id] = output;
}
