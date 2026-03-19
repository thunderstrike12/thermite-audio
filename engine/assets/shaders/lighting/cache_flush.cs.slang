/**
 * @name Cache Flush [1 thread per pixel]
 * This pass flushes the spatial accumulators in the cache.
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

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[thread_id]);
    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return;

    /* Flush the cache slot */
    cache::flush(macrofacet_cache, facet_key);
}
