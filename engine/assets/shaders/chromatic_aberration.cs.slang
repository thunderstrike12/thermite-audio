import view;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Post tonemap buffer. */
Texture2D<float3> post_tonemap;
/* Linear Sampler */
SamplerState linear_sampler;
/* Render Target. */
RWTexture2D<float4> render_target;

struct Constants {
    float3 rgb_offsets;
    float strength;
};

[[vk::push_constant]]
Constants ca;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    if (ca.strength == 0.0f) {
        render_target[thread_id] = float4(post_tonemap[thread_id], 1.0f);
        return;
    }

    const float2 res = float2(render_view.resolution);
    const float2 uv = (float2(thread_id) + 0.5f) / res;

    const float2 center = float2(0.5f);
    const float2 dir = uv - center;

    const float edge_dist = min(min(uv.x, 1.0f - uv.x), min(uv.y, 1.0f - uv.y));
    const float edge_fade = saturate(edge_dist * 20.0f);  // fade over ~5% of screen

    const float2 uv_r = uv + dir * ca.strength * ca.rgb_offsets.r * edge_fade;
    const float2 uv_g = uv + dir * ca.strength * ca.rgb_offsets.g * edge_fade;
    const float2 uv_b = uv + dir * ca.strength * ca.rgb_offsets.b * edge_fade;

    const float r = post_tonemap.SampleLevel(linear_sampler, uv_r, 0).r;
    const float g = post_tonemap.SampleLevel(linear_sampler, uv_g, 0).g;
    const float b = post_tonemap.SampleLevel(linear_sampler, uv_b, 0).b;

    render_target[thread_id] = float4(r, g, b, 1.0f);
}
