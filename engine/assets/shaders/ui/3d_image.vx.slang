import view;
import ui;

ConstantBuffer<RenderView> gpu_view;
StructuredBuffer<ImageInstance3D> images;

[shader("vertex")]
VertexOutput main(VertexInput input, int instance_id: SV_InstanceID) {
    const ImageInstance3D instance = images[instance_id];

    VertexOutput output;
    const float4 world = mul(instance.local_to_world, float4(input.pos, 1.0f));
    output.pos = mul(gpu_view.world_to_clip, world);
    output.pos.xy -= gpu_view.jitter * output.pos.w; /* undo TAA jitter */
    output.uvs = input.uvs;
    output.instance_id = instance_id;
    return output;
}
