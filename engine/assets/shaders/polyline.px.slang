/* Vertex shader output. */
struct VertexOut {
    float4 clip_pos : SV_POSITION;
    float4 color : COLOR;
};

[shader("fragment")]
float4 main(VertexOut input) : SV_Target0 {
    return input.color;
}
