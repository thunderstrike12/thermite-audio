import view;
import visibility;
import math;
import voxel;
import cache;
import colorspace;

/* Active render view */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer */
Texture2D<uint2> vbuffer;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_illuminance_cache;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[thread_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
        return;
    }

    /* Sample the illuminance cache */
    const float3 illuminance = cache::sample(macrofacet_illuminance_cache, facet_key, render_view.frame_index);
    render_target[thread_id] = float4(cs::delinearize(cs::acescg_to_r709(cs::aces_filmic_tonemapping(illuminance))), 1.0f); /* TEMP: Remove tonemapping! */
}
