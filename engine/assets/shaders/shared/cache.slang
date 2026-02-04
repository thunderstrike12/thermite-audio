module cache;

import math;
import voxel;

namespace cache {

/* Fuse length for the cache read/write operations. */
public static const uint FUSE_LENGTH = 16u;
/* Empty slot key. */
public static const uint64_t EMPTY_KEY = 0ull;
/* Number of inactive frames after which to free a slot. */
public static const uint INACTIVE_THRESHOLD = 240u;
/* Number of slots available in the cache. */
static const uint CAPACITY = 10000000u;

/* Illuminance Cache Slot */
public struct Slot {
    public uint64_t key; /* 64-bit hash key */
    public uint illuminance_r;
    public uint illuminance_g;
    public uint illuminance_b;
    public uint samples; /* sample count & last sampled frame index */

    /* Samples bits */
    static const uint SAMPLE_COUNT_BITS = 20u;
    static const uint SAMPLE_COUNT_MASK = (1u << SAMPLE_COUNT_BITS) - 1u;
    static const uint LAST_SAMPLED_BITS = 32u - SAMPLE_COUNT_BITS;
    static const uint LAST_SAMPLED_MASK = (1u << LAST_SAMPLED_BITS) - 1u;

    /* Number of accumulated samples in the cache slot. */
    public property uint sample_count { get {
        return samples & SAMPLE_COUNT_MASK;
    }}

    /* Last frame index this cache slot was sampled. */
    public property uint last_sampled { get {
        return samples >> SAMPLE_COUNT_BITS;
    }}

    /* Quantization factor */
    static const float QFACTOR = 1e3f;
    static const float RCP_QFACTOR = 1.0f / QFACTOR;

    /* The mean illuminance of the cache slot. */
    public property float3 illuminance { get {
        return float3(
            (float)illuminance_r * RCP_QFACTOR,
            (float)illuminance_g * RCP_QFACTOR,
            (float)illuminance_b * RCP_QFACTOR
        ) / sample_count;
    }}

    /* Find the inactivity of the cache slot compared to a reference frame index. */
    public [ForceInline] uint inactivity(const uint ref_frame) {
        const uint current_index = ref_frame & LAST_SAMPLED_MASK;
        const uint sampled_index = last_sampled;
        return ((current_index - sampled_index) << SAMPLE_COUNT_BITS) >> SAMPLE_COUNT_BITS;
    }

    /* Activate the cache slot for the given frame index. */
    public [ForceInline] [mutating] void activate(const uint frame_index) {
        samples = sample_count | (frame_index << SAMPLE_COUNT_BITS);
    }

    /* Accumulate into the cache slot. */
    public [ForceInline] [mutating] void accumulate(const float3 value) {
        /* Avoid accumulating too many samples, we can only handle ~1.000.000 */
        if (sample_count > 100000u) return;

        /* Convert to fixed point representation for atomics */
        const uint3 fxp = uint3(value * QFACTOR);

        /* Avoid atomics if the value is zero */
        if (fxp.r > 0u) InterlockedAdd(illuminance_r, fxp.r);
        if (fxp.g > 0u) InterlockedAdd(illuminance_g, fxp.g);
        if (fxp.b > 0u) InterlockedAdd(illuminance_b, fxp.b);
        InterlockedAdd(samples, 1u);
    }

    /* Reset the cache slot so it can be re-used. */
    public [ForceInline] [mutating] void reset() {
        key = EMPTY_KEY;
        illuminance_r = illuminance_g = 0u;
        illuminance_b = samples = 0u;
    }

    /* How many samples must be accumulated before we normalize illuminance. */
    static const uint NORM_THRESHOLD = 1u << 9u;
    /* How many samples do we normalize into. */
    static const uint NORM_COUNT = 1u << 8u;

    /* Normalize the accumulated illuminance in the cache slot. */
    public [ForceInline] [mutating] void normalize() {
        if (sample_count <= NORM_THRESHOLD) return;
        const uint last_sampled_index = last_sampled;

        /* Reduce sample count back to NORM_COUNT */
        const uint3 fxp = uint3(illuminance * NORM_COUNT * QFACTOR);
        illuminance_r = fxp.r;
        illuminance_g = fxp.g;
        illuminance_b = fxp.b;
        samples = NORM_COUNT | (last_sampled_index << SAMPLE_COUNT_BITS);
    }
};

/* Insert illuminance into the illuminance cache. */
public [ForceInline] void insert(RWStructuredBuffer<cache::Slot> cache, const uint64_t key, const float3 illuminance) {
    /* Modulo the hash to fall within the cache capacity */
    uint slot = voxel_hash(key) % CAPACITY;

    /* Find or insert the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which already exists matching our key */
        if (cache[slot].key == key) {
            cache[slot].accumulate(illuminance);
            break;
        }

        /* If the current slot is empty */
        if (cache[slot].key == EMPTY_KEY) {
            uint64_t original_key = 0u; /* Try to claim this slot */
            InterlockedCompareExchange(cache[slot].key, EMPTY_KEY, key, original_key);

            /* If instead this thread succeeded in claiming the slot */
            if (original_key == EMPTY_KEY) {
                cache[slot].accumulate(illuminance);
                break;
            }
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % CAPACITY;
    }
}

/* Sample illuminance from the illuminance cache. (returns BIG_F32 if no sample was found) */
public [ForceInline] float3 sample(RWStructuredBuffer<cache::Slot> cache, const uint64_t key, const uint frame_index) {
    /* Modulo the hash to fall within the cache capacity */
    uint slot = voxel_hash(key) % CAPACITY;

    /* Find the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which matches our key */
        if (cache[slot].key == key) {
            cache[slot].activate(frame_index);
            return cache[slot].illuminance;
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % CAPACITY;
    }

    return 0.0f; /* Couldn't find a sample */
}

} // cache
