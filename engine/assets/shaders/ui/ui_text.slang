module ui_text;

/* Vertex input */
public struct TextVertexInput {
    public float3 pos;
    public float2 uvs;
};

/* Vertex output */
public struct TextVertexOutput {
    public float4 pos : SV_Position;
    public float2 uvs : TEX_COORD0;
    public int instance_id;
};

/* Data for a single glyph instance. */
public struct GlyphInstance {
    /* Position and size */
    public float4 pos_size; /* .xy = screen position, .zw = glyph size */
    /* Font atlas UV rectangle. */
    public float4 uv_rect; /* .xy = min UV, .zw = max UV */
    /* Text color (RGBA Rec.709) */
    public float4 color;
    /* SDF parameters */
    public float4 params; /* .x = atlas_index, .yzw = unused */
    /* Euler angles */
    public float3 angles;
    
    public float2 pos() { return pos_size.xy; }
    public float2 size() { return pos_size.zw; }
    public float2 uv_min() { return uv_rect.xy; }
    public float2 uv_max() { return uv_rect.zw; }
    public uint atlas_index() { return asuint(params.x); }
};
