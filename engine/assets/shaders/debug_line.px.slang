import common;

[shader("fragment")]
float4 main(VertexOutput input) : SV_Target0 {
    return float4(input.color, 1.0f);
}