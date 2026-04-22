import bindless;
import ui;

[[vk::binding(1, 0)]]
StructuredBuffer<ImageInstance> images;
[[vk::binding(2, 0)]]
SamplerState image_sampler;

[shader("pixel")]
float4 main(VertexOutput input) : SV_Target {
    /* Fetch the image instance this fragment belongs to */
    const ImageInstance instance = images[input.instance_id];

    /* Fetch the texture to render onto this image */
    const uint32_t image_index = instance.image_index();
    if (image_index == 0x00u) return instance.color();

    const Texture2D<float4> sampler = bindless::texture_sampled<Texture2D<float4>>(image_index);

    /* Compute flipbook UVs */
    const float frame = floor(instance.flipbook_frame());
    const float2 anim_uv = float2((frame + input.uvs.x) / instance.flipbook_frames(), input.uvs.y);

    /* Sampler the texture */
    return sampler.SampleLevel(image_sampler, anim_uv, 0) * instance.color();
}
