import view;
import ui_text;

/* GPU resources */
ConstantBuffer<RenderView> gpu_view;
StructuredBuffer<GlyphInstance3D> glyphs;

[shader("vertex")]
TextVertexOutput main(TextVertexInput input, int instance_id: SV_InstanceID) {
    /* Get the glyph instance data */
    const GlyphInstance3D glyph = glyphs[instance_id];

    /* Interpolate UVs from glyph's UV rect */
    const float2 uvs = lerp(glyph.uv_min(), glyph.uv_max(), float2(input.uvs.x, 1.0f - input.uvs.y));
    
    /* Output */
    TextVertexOutput output;
    const float4 world = mul(glyph.local_to_world, float4(float2(glyph.pos().x, -glyph.pos().y - glyph.size().y) + input.pos.xy * glyph.size(), 0.0f, 1.0f));
    output.pos = mul(gpu_view.world_to_clip, world);
    output.pos.xy -= gpu_view.jitter * output.pos.w; /* undo TAA jitter */
    output.uvs = uvs;
    output.instance_id = instance_id;
    return output;
}
