/**
 * @name Macrofacet Reset [1 thread per hash cell]
 * ...
 */
import view;

static const uint64_t EMPTY_KEY = 0ull;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Macrofacet hash set. */
RWStructuredBuffer<uint64_t> macrofacet_hashset;
/* Macrofacet shading commands buffer. */
/* NOTE: First 4 uints are indirect dispatch args, after that uint64_t commands. */
RWStructuredBuffer<uint> macrofacet_shading_commands;

[shader("compute")]
[numthreads(128, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (thread_id >= (render_view.resolution.x * render_view.resolution.y)) return;

    /* Reset hash cell */
    macrofacet_hashset[thread_id] = EMPTY_KEY;

    /* Reset indirect dispatch args */
    if (thread_id == 0) {
        macrofacet_shading_commands[0] = 0u;
        macrofacet_shading_commands[1] = 1u;
        macrofacet_shading_commands[2] = 1u;
        macrofacet_shading_commands[3] = 0u;
    }
}
