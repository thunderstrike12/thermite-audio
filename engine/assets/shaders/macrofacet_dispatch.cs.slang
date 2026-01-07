/**
 * @name Macrofacet Dispatch [1 thread per pixel]
 * This pass is responsible for generation macrofacet shading commands based on what's visible on screen.
 * Only one shading command can be dispatched per macrofacet (voxel face)
 * This is important to avoid race conditions when writing to the cache in a following pass.
 * To achieve this we use a hash set with 64 bit atomics to find all unique macrofacets on screen.
 */
import voxel;
import visibility;
import view;
import math;

static const uint FUSE_LENGTH = 32u;
static const uint64_t EMPTY_KEY = 0ull;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer. */
RWTexture2D<uint2> vbuffer;
/* Macrofacet hash set. */
RWStructuredBuffer<uint64_t> macrofacet_hashset;
/* Macrofacet shading commands buffer. */
/* NOTE: First 4 uints are indirect dispatch args, after that uint64_t commands. */
RWStructuredBuffer<uint> macrofacet_shading_commands;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[thread_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return;
    /* Unpack the key into its voxel components */
    const VisData v = unpack_visibility(facet_key);

    /* Evaluate the voxel hash function */
    const uint capacity = render_view.resolution.x * render_view.resolution.y;
    uint slot = voxel_hash(v.voxel_index, v.object_index, v.face_index) % capacity;

    /* Find or insert the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which already exists matching our key */
        if (macrofacet_hashset[slot] == facet_key) break;

        /* If the current slot is empty */
        if (macrofacet_hashset[slot] == EMPTY_KEY) {
            uint64_t original_key = 0u; /* Try to claim this slot */
            InterlockedCompareExchange(macrofacet_hashset[slot], EMPTY_KEY, facet_key, original_key);

            /* Exit if another thread succeeded in claiming the slot with the same key */
            if (original_key == facet_key) break;

            /* If instead this thread succeeded in claiming the slot */
            /* Dispatch a new shading command for this macrofacet */
            if (original_key == EMPTY_KEY) {
                uint command_index = 0u; /* Claim a new shading command index */
                InterlockedAdd(macrofacet_shading_commands[3], 1u, command_index);

                /* If required, update the indirect dispatch args */
                if ((macrofacet_shading_commands[0] * 128u) < command_index) {
                    macrofacet_shading_commands[0] = (command_index + 127u) / 128u;
                }

                /* Insert the macrofacet shading command */
                const uint2 facet = asuint2(facet_key);
                macrofacet_shading_commands[4u + command_index * 2u + 0u] = facet.x;
                macrofacet_shading_commands[4u + command_index * 2u + 1u] = facet.y;
                break;
            }
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % capacity;
    }
}
