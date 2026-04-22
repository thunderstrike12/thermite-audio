import lightgrid;

RWStructuredBuffer<uint32_t> light_grid;

/* Maximum valid ID for bounds check. */
static const uint MAX_ID = lgrid::CASCADE_RESOLUTION3 * lgrid::CASCADE_COUNT;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (thread_id >= MAX_ID) return;
    light_grid[thread_id * lgrid::CELL_STRIDE] = 0u;
}
