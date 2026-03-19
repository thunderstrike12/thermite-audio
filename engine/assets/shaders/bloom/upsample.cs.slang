import view;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Down Sampler */
SamplerState down_sampler;
/* Mip imaage views. */
RWTexture2D<float3> curr_mip;
Texture2D<float3> prev_mip;

struct UpsampleConstants {
    uint32_t curr_mip_level;
    float bloom_radius;
};
[[vk::push_constant]]
UpsampleConstants constants;

static const float2 COORDS[9] = float2[9](
	float2(-1.0f, 1.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f),
	float2(-1.0f, 0.0f), float2(0.0f, 0.0f), float2(1.0f, 0.0f),
	float2(-1.0f, -1.0f), float2(0.0f, -1.0f), float2(1.0f, -1.0f)
);

static const float WEIGHTS[9] = float[9](
	0.0625f, 0.125f, 0.0625f,
	0.125f, 0.25f, 0.125f,
	0.0625f, 0.125f, 0.0625f
);

[shader("compute")]
[numthreads(16, 8, 1)]
void main(uint2 thread_id: SV_DispatchThreadID) {
    /* Current (destination) mip dimensions. */
    const float2 dst_res = float2(render_view.resolution >> constants.curr_mip_level);
    /* Source (next) mip dimensions. */
    const float2 src_res = float2(render_view.resolution >> (constants.curr_mip_level + 1));

    /* Early out for threads beyond the current mip's extent. */
    if (any(thread_id >= uint2(dst_res)))
        return;

    const float2 uv = (float2(thread_id) + 0.5f) / dst_res;
    const float2 src_texel_size = 1.0f / src_res;
    float3 previous = float3(0.0f);

    for (int i = 0; i < 9; i++) {
        float2 current_uv = uv + COORDS[i] * src_texel_size;
        previous += WEIGHTS[i] * prev_mip.SampleLevel(down_sampler, current_uv, 0.0f);
    }

    curr_mip[thread_id] = lerp(curr_mip[thread_id], previous, constants.bloom_radius);
}
