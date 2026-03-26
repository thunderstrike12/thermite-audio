import view;
import ui_text;

/* GPU resources */
ConstantBuffer<RenderView> gpu_view;
StructuredBuffer<GlyphInstance> glyphs;

/* Rotate the point 'p' on the Z axis. */
inline float2 rotate_vertex(const float2 p, const GlyphInstance instance) {
    const float2 o = p; //- instance.extent() * instance.pivot();

    /* <https://stackoverflow.com/questions/34050929/3d-point-rotation-algorithm> */
    const float cosa = cos(instance.angles.z);
    const float sina = sin(instance.angles.z);
    const float cosb = cos(instance.angles.y);
    const float sinb = sin(instance.angles.y);
    const float cosc = cos(instance.angles.x);
    const float sinc = sin(instance.angles.x);
    const float xx = cosa * cosb;
    const float xy = cosa * sinb * sinc - sina * cosc;
    const float yx = sina * cosb;
    const float yy = sina * sinb * sinc + cosa * cosc;
    return float2(xx * o.x + xy * o.y, yx * o.x + yy * o.y);
}

[shader("vertex")]
TextVertexOutput main(TextVertexInput input, int instance_id: SV_InstanceID) {
    /* Get the glyph instance data */
    const GlyphInstance glyph = glyphs[instance_id];

    /* Convert to normalized device coordinates (same as ui_image shader) */
    const float2 ndc = (glyph.pos() + rotate_vertex(input.pos.xy * glyph.size(), glyph)) / gpu_view.resolution;
    
    /* Interpolate UVs from glyph's UV rect */
    const float2 uvs = lerp(glyph.uv_min(), glyph.uv_max(), input.uvs);
    
    /* Output */
    TextVertexOutput output;
    output.pos = float4(ndc * 2.0f - 1.0f, 0.0f, 1.0f);
    output.uvs = uvs;
    output.instance_id = instance_id;
    return output;
}
