module ui;

public struct VertexInput {
    public float3 pos;
    public float2 uvs;
};

public struct VertexOutput {
    public float4 pos : SV_Position;
    public float2 uvs : TEX_COORD0;
    public int instance_id;
};

/* Data we need per image instance */
public struct ImageInstance {
    float4 pos_extent;       /* .xy = position    | .zw = extent */
    float4 color_rgba;       /* .xyzw = color */
    float4 angle_index;      /* .xyz = xyz angles | .w = image_index */
    float4 pivot_flipbook;   /* .xy = pivot | .z = flipbook_frame | .w = flipbook_frames */

    /* Unpacking functions */
    public inline float2 pos() { return pos_extent.xy; }
    public inline float2 extent() { return pos_extent.zw; }
    public inline float4 color() { return color_rgba; }
    public inline uint32_t image_index() { return asuint(angle_index.w); }
    public inline float3 angles() { return angle_index.xyz; }
    public inline float2 pivot() { return pivot_flipbook.xy; }
    public inline uint32_t flipbook_frame() { return asuint(pivot_flipbook.z); }
    public inline uint32_t flipbook_frames() { return asuint(pivot_flipbook.w); }
};
