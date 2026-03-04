import cache;

/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;

[shader("compute")]
[numthreads(128, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (thread_id >= cache::CAPACITY) return;

    /* Initialize the cache slot */
    macrofacet_cache[thread_id].reset();
}
