import common;

[shader("vertex")]
VertexOutput main(VertexInput input) {
    VertexOutput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}