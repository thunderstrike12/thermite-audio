import view;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Down Sampler */
SamplerState down_sampler;
/* Mip imaage views. */
RWTexture2D<float3> curr_mip;
Texture2D<float3> prev_mip;

[[vk::push_constant]]
 uint32_t curr_mip_level;

static const float2 COORDS[13] = float2[13](
    float2(-1.0f, 1.0f), float2(1.0f, 1.0f), float2(-1.0f, -1.0f), float2(1.0f, -1.0f),

    float2(-2.0f, 2.0f), float2(0.0f, 2.0f), float2(2.0f, 2.0f), float2(-2.0f, 0.0f), float2(0.0f, 0.0f), float2(2.0f, 0.0f), float2(-2.0f, -2.0f), float2(0.0f, -2.0f), float2(2.0f, -2.0f)
);

float max_brightness(float3 Color)
{
    return max(max(Color.r, Color.g), Color.b);
}

float3 avg_block(float3 sample1, float3 sample2, float3 sample3, float3 sample4) {
    const float4 sample_brightness = float4(max_brightness(sample1), max_brightness(sample2), max_brightness(sample3), max_brightness(sample4));

    const float4 weights = 1.0f / (sample_brightness + 1.0f);
    const float weight_sum = weights.x + weights.y + weights.z + weights.w;

    return (sample1 * weights.x + sample2 * weights.y + sample3 * weights.z + sample4 * weights.w) * (1.0f / weight_sum);
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(uint2 thread_id: SV_DispatchThreadID) {
    /* Current mip dimensions. */
    const float2 dst_res = float2(render_view.resolution >> curr_mip_level);
    /* Source (previous) mip dimensions. */
    const float2 src_res = float2(render_view.resolution >> (curr_mip_level - 1));

    /* Early out for threads beyond the current mip's extent. */
    if (any(thread_id >= uint2(dst_res)))
        return;

    const float2 uv = (float2(thread_id) + 0.5f) / dst_res;
    const float2 src_texel_size = 1.0f / src_res;

    float3 samples[13];
    for (int i = 0; i < 13; i++) {
        float2 sample_uv = uv + COORDS[i] * src_texel_size;
        samples[i] = prev_mip.SampleLevel(down_sampler, sample_uv, 0.0f).rgb;
    }

    float3 center = avg_block(samples[0], samples[1], samples[2], samples[3]);
    float3 top_left = avg_block(samples[4], samples[5], samples[8], samples[7]);
    float3 top_right = avg_block(samples[5], samples[6], samples[9], samples[8]);
    float3 bottom_left = avg_block(samples[7], samples[8], samples[11], samples[10]);
    float3 bottom_right = avg_block(samples[8], samples[9], samples[12], samples[11]);

    curr_mip[thread_id] = 0.5f * center + 0.125f * (top_left + top_right + bottom_left + bottom_right);
}
