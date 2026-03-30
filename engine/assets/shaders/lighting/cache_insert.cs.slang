/**
 * @name Cache Insert [1 thread per pixel]
 * This pass inserts luminance into the spatial accumulators in the cache.
 */
import math;
import view;
import cache;
import visibility;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Diffuse & Specular buffers. */
Texture2D<float3> diffuse;
Texture2D<float3> specular;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[thread_id]);
    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return;

    /* Quarter-rate shading */
    const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
    const int2 pixel_id = ((thread_id >> 1) << 1) + offset;

    /* Insert luminance into the cache */
    if (all(thread_id == pixel_id)) {
        cache::insert(macrofacet_cache, facet_key, diffuse[thread_id], specular[thread_id >> 1] * 4.0f);
    } else {
        cache::insert(macrofacet_cache, facet_key, diffuse[thread_id], 0.0f);
    }
}
