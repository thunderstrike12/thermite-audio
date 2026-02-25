import bindless;
import particles;

[[vk::binding(3, 0)]]
SamplerState linear_sampler;

[shader("fragment")]
float4 main(BillboardVertexOutput input) : SV_Target0 {
    Texture2D<float4> tex = bindless::texture_sampled<Texture2D<float4>>(input.tex_index);

    float4 tex_sample = tex.Sample(linear_sampler, input.uv);

    return float4(tex_sample.rgb, input.opacity);
}