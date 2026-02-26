/**
 * @name Cache Eviction [1 thread per 8 hash cells]
 * This pass evicts macrofacets from the illuminance cache if they are inactive for too long.
 * This pass also normalizes illuminance stored in the cache, to avoid overflowing.
 */
import math;
import view;
import cache;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;

static const uint CAPACITY = 10000000u;
// static const uint THREADS = (CAPACITY + 8u - 1u) / 8u;

[shader("compute")]
[numthreads(128, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Find the id of the slot for the current thread */
    // const int offset = THREADS * (render_view.frame_index & 0b111u);
    // const int slot_id = offset + thread_id;
    const int slot_id = thread_id;

    /* Bounds check */
    if (slot_id >= CAPACITY) return;

    /* Skip empty slots */
    if (macrofacet_cache[slot_id].key == cache::EMPTY_KEY) return;

    /* Find the number of frames between the current and last sampled frame */
    const uint inactivity = macrofacet_cache[slot_id].inactivity(render_view.frame_index);

    /* If this slot has been inactive for too long, reset it so it can be re-used */
    if (inactivity > cache::INACTIVE_THRESHOLD) {
        macrofacet_cache[slot_id].reset();
        return;
    }

    /* Flush the spatial accumulator in the cache slot */
    macrofacet_cache[slot_id].flush_accu(inactivity);
}
