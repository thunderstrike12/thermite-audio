import bindless;
import ui_text;

/* GPU resources */
[[vk::binding(1, 0)]]
StructuredBuffer<GlyphInstance> glyphs;
[[vk::binding(2, 0)]]
SamplerState text_sampler;

/* MSDF median */
[ForceInline]
float median(const float3 msdf) {
    return max(min(msdf.r, msdf.g), min(max(msdf.r, msdf.g), msdf.b));
}

float screen_px_range(Texture2D<float4> atlas, float2 uv, float px_range) {
    uint aw, ah;
    atlas.GetDimensions(aw, ah);
    const float2 unit_range = float2(px_range, px_range) / float2(aw, ah);
    const float2 screen_tex_size = float2(1.0f, 1.0f) / fwidth(uv);
    return max(0.5f * dot(unit_range, screen_tex_size), 1.0f);
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

    const float spr = screen_px_range(atlas, input.uvs, 16.0f /* font.px_range */);
    const float dist_px = (sd - 0.5f) * spr;         // signed, + = inside glyph
    const float opacity = saturate(dist_px + 0.5f);  // 1px AA, exact

    float3 rgb = float3(0.0f);
    float alpha = 0.0f;
    if (glyph.glow_strength() > 0.0f) {
        const float radius = max(glyph.glow_radius(), 1e-3f);
        const float range_px = 0.5f * spr;        // SDF data boundary, in screen px
        const float out_px = max(0.0f, -dist_px); // 0 at edge, range_px on the plateau

        float glow_a = exp(-out_px / radius);

        /* Small-text compensation (unchanged) */
        const float size_boost = saturate(radius / range_px - 1.0f);
        glow_a *= 1.0f + size_boost * glyph.glow_boost();

        /* Force the halo to vanish where the distance field goes flat. Without this,
           the saturated sd==0 region (corners, far field) carries a constant glow
           value clipped hard at the rectangular quad edge — the visible box. */
        glow_a *= 1.0f - smoothstep(range_px * 0.6f, range_px, out_px);

        const float glow_alpha = saturate(glow_a * glyph.glow_strength());
        const float3 glow_pm = glyph.glow_color.rgb * glow_alpha;
        const float3 core_pm = glyph.color.rgb * opacity;
        const float core_alpha = opacity;
        const float3 rgb_pm = core_pm + (1.0f - core_alpha) * glow_pm;
        const float a = core_alpha + (1.0f - core_alpha) * glow_alpha;
        rgb = rgb_pm * glyph.color.a;
        alpha = a * glyph.color.a;
    } else {
        rgb = glyph.color.rgb * opacity * glyph.color.a;
        alpha = opacity * glyph.color.a;
    }

    return float4(rgb, alpha);
}
