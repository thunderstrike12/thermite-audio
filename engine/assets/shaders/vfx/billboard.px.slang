import bindless;
import particles;
import math;
import view;
import colorspace;
import volume;

ConstantBuffer<RenderView> gpu_view;

[[vk::binding(3, 0)]]
Texture2D<float4> blue_noise1d;
[[vk::binding(4, 0)]]
SamplerState point_sampler;

[[vk::binding(5, 0)]] 
Texture3D<float3> froxel_luminance;
[[vk::binding(6, 0)]]
SamplerState froxel_sampler;

struct FragmentOutput {
    float4 color : SV_Target0;
    float2 motion : SV_Target1;
};

[shader("fragment")]
FragmentOutput main(BillboardVertexOutput input) {
    const float noise = r1(blue_noise1d.Sample(point_sampler, input.pos.xy / (512.0f * input.dither_scale)).r, gpu_view.frame_index % 1024u);

    // skip texture fetch if opacity fails dither test
    if (input.opacity <= noise)
        discard;

    Texture2D<float4> tex = bindless::texture_sampled<Texture2D<float4>>(input.tex_index);

    const float frame = floor(input.current_frame);
    const float2 anim_uv = float2((frame + input.uv.x) / input.flipbook_frames, input.uv.y);

    const float4 tex_sample = tex.Sample(point_sampler, anim_uv);

    // dither when alpha is above 0.0f but below 1.0f
    if (tex_sample.a * input.opacity <= noise)
        discard;

    FragmentOutput output;

    /* Sample froxel volume for lighting data */
    const float3 uvw = iv::froxel_uvw(input.world_pos, gpu_view.world_to_clip, iv::FROXEL_NEAR_Z, iv::FROXEL_FAR_Z);
    const float3 li = froxel_luminance.SampleLevel(froxel_sampler, uvw, 0) + (tex_sample.rgb * input.emission);

    output.color = float4(cs::r709_to_acescg(cs::linearize(tex_sample.rgb * li)), 1.0f);
    output.motion = input.motion_vec;

    return output;
}
