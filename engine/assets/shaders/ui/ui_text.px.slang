import bindless;
import ui_text;

/* GPU resources */
[[vk::binding(1, 0)]]
StructuredBuffer<GlyphInstance> glyphs;
[[vk::binding(2, 0)]]
SamplerState text_sampler;

/* MSDF median */
[ForceInline] float median(const float3 msdf) {
    return max(min(msdf.r, msdf.g), min(max(msdf.r, msdf.g), msdf.b));
}

[shader("pixel")]
float4 main(TextVertexOutput input) : SV_Target {
    /* Get glyph instance data */
    const GlyphInstance glyph = glyphs[input.instance_id];
    
    /* Get the font atlas texture using bindless */
    const uint atlas_idx = glyph.atlas_index();
    if (atlas_idx == 0u) {
        /* No texture, return solid color (shouldn't happen with proper fonts) */
        return glyph.color;
    }
    const Texture2D<float4> atlas = bindless::texture_sampled<Texture2D<float4>>(atlas_idx);

    /* Read and interpret multi-channel SDF */
    const float3 msdf = atlas.Sample(text_sampler, input.uvs).rgb;
    const float sd = median(msdf);
    const float opacity = smoothstep(0.5f - fwidth(sd), 0.5f + fwidth(sd), sd);
    return float4(glyph.color.rgb, glyph.color.a * opacity);
}
