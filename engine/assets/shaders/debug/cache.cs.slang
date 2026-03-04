import view;
import visibility;
import math;
import voxel;
import cache;
import debug;

/* Active render view */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer */
Texture2D<uint2> vbuffer;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[thread_id]);

    if (null_visibility(packed)) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
    } else {
        /* Modulo the hash to fall within the cache capacity */
        uint slot = voxel_hash(packed) % cache::CAPACITY;

        /* Find the macrofacet */
        for (uint fuse = 0u; fuse < cache::FUSE_LENGTH; ++fuse) {
            /* Exit if we found a slot which matches our key */
            if (macrofacet_cache[slot].key == packed) {
                render_target[thread_id] = float4(heat_color((float)fuse / cache::FUSE_LENGTH), 1.0f);
                macrofacet_cache[slot].activate(render_view.frame_index);
                return;
            }

            /* Move to the next hash cache slot */
            slot = (slot + 1u) % cache::CAPACITY;
        }

        render_target[thread_id] = float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
}
