import view;
import visibility;
import math;
import voxel;
import debug;

static const uint FUSE_LENGTH = 32u;
static const uint64_t EMPTY_KEY = 0ull;

struct HashCell {
    uint64_t key;
    uint value;
};

/* Active render view */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer */
Texture2D<uint2> vbuffer;
/* Macrofacet illuminance cache buffer. */
StructuredBuffer<HashCell> macrofacet_illuminance_cache;
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
    /* Unpack the key into its voxel components */
    const VisData v = unpack_visibility(facet_key);

    /* Evaluate the voxel hash function */
    const uint capacity = 10000000u;
    uint slot = voxel_hash(v.voxel_index, v.object_index, v.face_index) % capacity;

    /* Find the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which matches our key */
        if (macrofacet_illuminance_cache[slot].key == facet_key) {
            render_target[thread_id] = float4(decode_rgb9e5(macrofacet_illuminance_cache[slot].value), 1.0f);
            return;
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % capacity;
    }

    render_target[thread_id] = float4(1.0f, 0.0f, 0.0f, 1.0f);
}
