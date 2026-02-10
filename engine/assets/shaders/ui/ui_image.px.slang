import bindless;
import ui;

[[vk::binding(1, 0)]]
StructuredBuffer<ImageInstance> images;

[shader("pixel")]
float4 main(VertexOutput input) : SV_Target {
    /* Fetch the image instance this fragment belongs to */
    const ImageInstance instance = images[input.instance_id];

    /* Fetch the texture to render onto this image */
    const uint32_t image_index = instance.image_index();
    if (image_index == 0x00u) return instance.color();

    const Sampler2D<float4> sampler = bindless::texture_sampled<Sampler2D<float4>>(image_index);

    /* Sampler the texture */
    return sampler.SampleLevel(input.uvs, 0) * instance.color();
}