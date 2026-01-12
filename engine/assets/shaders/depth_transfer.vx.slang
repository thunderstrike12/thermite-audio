/* Vertex shader output. */
struct VertexOut {
    float4 clip_pos : SV_POSITION;
};

[shader("vertex")]
VertexOut main(uint vertex_id: SV_VertexID) {
    const float2 uv = float2(vertex_id & 1u, (vertex_id >> 1) & 1u);
    return VertexOut(float4(uv * 4.0f - 1.0f, 0.0f, 1.0f));
}
