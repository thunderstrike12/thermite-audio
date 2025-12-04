module common;

public struct VertexOutput {
    public float4 position : SV_Position;
    public float3 color : COLOR0;
};

public struct VertexInput {
    public float3 position : POSITION;
    public float3 color : COLOR;
};