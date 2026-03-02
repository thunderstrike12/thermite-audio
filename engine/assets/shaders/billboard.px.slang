import bindless;
import particles;

[[vk::binding(3, 0)]]
Texture2D<float4> blue_noise1d;

[[vk::binding(4, 0)]]
SamplerState point_sampler;

[shader("fragment")]
float4 main(BillboardVertexOutput input) : SV_Target0 {
    Texture2D<float4> tex = bindless::texture_sampled<Texture2D<float4>>(input.tex_index);

    const float frame = floor(input.current_frame);
    const float2 anim_uv = float2((frame + input.uv.x) / input.flipbook_frames, input.uv.y);

    const float4 tex_sample = tex.Sample(point_sampler, anim_uv);

    const float2 screen_pos = input.pos.xy;
    if (input.opacity <= 0.99f) {
        const float dither = blue_noise1d.Sample(point_sampler, screen_pos / (512.0f * input.dither_scale)).r;

        if (input.opacity < dither)
            discard;
    }

    if (tex_sample.a <= 0.0f)
        discard;

    return float4(tex_sample.rgb, 1.0f);
}