import common;
import view;

ConstantBuffer<RenderView> render_view;

[shader("vertex")]
VertexOutput main(VertexInput input) {
    VertexOutput output;
    output.position = mul(render_view.world_to_clip, float4(input.position, 1.0f));
    output.color = input.color;
    return output;
}