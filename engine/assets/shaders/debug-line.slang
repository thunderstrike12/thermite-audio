struct VertexInput {
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 color : COLOR0;
};

[shader("vertex")]
VertexOutput vertex_entry(VertexInput input) {
    VertexOutput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragment_entry(VertexOutput input) : SV_Target0 {
    return float4(input.color, 1.0f);
}