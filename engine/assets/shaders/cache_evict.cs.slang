/**
 * @name Cache Eviction [1 thread per hash cell]
 * This pass evicts macrofacets from the cache if they are inactive for too long.
 * This pass also flushes the spatial accumulators in the cache.
 */
import math;
import view;
import cache;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;

[shader("compute")]
[numthreads(128, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (thread_id >= cache::CAPACITY) return;

    /* Skip empty slots */
    if (macrofacet_cache[thread_id].key == cache::EMPTY_KEY) return;

    /* Find the number of frames between the current and last sampled frame */
    const uint inactivity = macrofacet_cache[thread_id].inactivity(render_view.frame_index);

    /* If this slot has been inactive for too long, reset it so it can be re-used */
    if (inactivity > cache::INACTIVE_THRESHOLD) {
        macrofacet_cache[thread_id].reset();
        return;
    }

    /* Flush the spatial accumulator in the cache slot */
    macrofacet_cache[thread_id].flush_accu(inactivity);
}
