/**
 * @name Macrofacet Direct Illumination (DI) [1 thread per macrofacet]
 * ...
 */
import view;
import math;
import visibility;
import voxel;
import bvh2;

static const uint FUSE_LENGTH = 32u;
static const uint64_t EMPTY_KEY = 0ull;

struct HashCell {
    uint64_t key;
    uint value;
};

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Macrofacet shading commands buffer. */
/* NOTE: First 4 uints are indirect dispatch args, after that uint64_t commands. */
StructuredBuffer<uint> macrofacet_shading_commands;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<HashCell> macrofacet_illuminance_cache;

/* TEMPORARY */
static const float3 LIGHT_LUMINANCE = float3(1.0f, 0.1f, 0.1f) * 256.0f;
static const float3 LIGHT_POS = float3(2.0f, 12.0f, 4.0f);
/* TEMPORARY */

[shader("compute")]
[numthreads(128, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (thread_id >= macrofacet_shading_commands[3]) return;

    /* Load macrofacet shading command */
    const uint64_t facet_key = asuint64(uint2(
        macrofacet_shading_commands[4 + thread_id * 2], 
        macrofacet_shading_commands[4 + thread_id * 2 + 1]
    ));

    /* Unpack visibility data */
    const VisData v = unpack_visibility(facet_key);
    const VoxelFacet facet = unpack_geometry(v, objects);

    /* Calculate light direction and distance */
    const float3 light_vec = LIGHT_POS - facet.center;
    const float light_dist_sq = dot(light_vec, light_vec);
    const float light_dist = sqrt(light_dist_sq);
    const float3 light_dir = light_vec / light_dist;

    /* Trace light occlusion */
    const Ray occlusion_ray = custom_ray(facet.center + facet.normal * 0.01f, light_dir);
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, occlusion_ray, light_dist);
    uint packed_illuminance = 0u;
    if (t >= light_dist) {
        /* Evaluate direct illumination */
        const float3 illuminance = LIGHT_LUMINANCE * max(0.0f, dot(facet.normal, light_dir)) / light_dist_sq;
        packed_illuminance = encode_rgb9e5(illuminance);
    }

    /* Evaluate the voxel hash function */
    const uint capacity = 10000000u;
    uint slot = voxel_hash(v.voxel_index, v.object_index, v.face_index) % capacity;

    /* Find or insert the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which already exists matching our key */
        if (macrofacet_illuminance_cache[slot].key == facet_key) {
            macrofacet_illuminance_cache[slot].value = packed_illuminance;
            break;
        }

        /* If the current slot is empty */
        if (macrofacet_illuminance_cache[slot].key == EMPTY_KEY) {
            uint64_t original_key = 0u; /* Try to claim this slot */
            InterlockedCompareExchange(macrofacet_illuminance_cache[slot].key, EMPTY_KEY, facet_key, original_key);

            /* If instead this thread succeeded in claiming the slot */
            if (original_key == EMPTY_KEY) {
                macrofacet_illuminance_cache[slot].value = packed_illuminance;
                break;
            }
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % capacity;
    }
}
