module cache;

import math;
import voxel;

namespace cache {

/* Fuse length for the cache read/write operations. */
public static const uint FUSE_LENGTH = 16u;
/* Empty slot key. */
public static const uint64_t EMPTY_KEY = 0xFFFFFFFFFFFFFFFFull;
/* Number of inactive frames after which to free a slot. */
public static const uint INACTIVE_THRESHOLD = 60u;
/* Number of slots available in the cache. */
public static const uint CAPACITY = 10000000u;

static const uint TEMPORAL_SAMPLES = 20u;

/* Cache Slot (48 bytes) */
public struct Slot {
    public uint64_t key; /* 64-bit hash key. */
    public uint sample_count; /* Spatial sample count. */
    public uint warmup_last; /* Warmup count & last sampled count. */
    public uint4 diffuse; /* Spatial & temporal diffuse illuminance. */
    public uint4 specular; /* Spatial & temporal specular luminance. */

    /* Samples bits */
    static const uint WARMUP_BITS = 16u;
    static const uint WARMUP_MASK = (1u << WARMUP_BITS) - 1u;
    static const uint LAST_SAMPLED_BITS = 32u - WARMUP_BITS;
    static const uint LAST_SAMPLED_MASK = (1u << LAST_SAMPLED_BITS) - 1u;

    /* Warmup counter for this cache slot. */
    public [ForceInline] uint warmup_counter() { return warmup_last & WARMUP_MASK; }

    /* Last frame index this cache slot was sampled. */
    public [ForceInline] uint last_sampled() { return warmup_last >> WARMUP_BITS; }
    public [ForceInline] [mutating] void set_last_sampled(uint value) { warmup_last = warmup_counter() | (value << WARMUP_BITS); }

    /* Quantization factor */
    static const float QFACTOR = 1e3f;
    static const float RCP_QFACTOR = 1.0f / QFACTOR;

    /* Read diffuse illuminance from the cache slot. */
    public [ForceInline] float3 read_diffuse() { return decode_rgb9e5(diffuse.w); }
    /* Read specular luminance from the cache slot. */
    public [ForceInline] float3 read_specular() { return decode_rgb9e5(specular.w); }

    /* Flush the diffuse and specular accumulators of the cache slot. */
    /* This resets the spatial accumulators and updates the temporal accumulators. */
    public [ForceInline] [mutating] void flush_accu() {
        /* Atomically claim the cache slot for flushing */
        uint spatial_samples = 0u;
        InterlockedExchange(sample_count, 0u, spatial_samples);
        if (spatial_samples == 0u) return;

        /* Calculate the mean of both spatial accumulators */
        const float rcp_sample_count = 1.0f / spatial_samples;
        const float3 diffuse_accu = float3(diffuse.rgb) * RCP_QFACTOR * rcp_sample_count;
        const float3 specular_accu = float3(specular.rgb) * RCP_QFACTOR * rcp_sample_count;

        /* Reset the spatial accumulators */
        diffuse.rgb = specular.rgb = 0u;

        /* Accumulator warmup factor */
        if (warmup_counter() < TEMPORAL_SAMPLES) warmup_last += 1u;

        const float alpha = 1.0f / (float)warmup_counter();
        diffuse.w = encode_rgb9e5(lerp(read_diffuse(), diffuse_accu, alpha));
        specular.w = encode_rgb9e5(lerp(read_specular(), specular_accu, alpha));
    }

    /* Find the inactivity of the cache slot compared to a reference frame index. */
    public [ForceInline] [mutating] uint inactivity(const uint frame_index) {
        const uint last = last_sampled();
        set_last_sampled(last + 1u);
        return last;
    }

    /* Activate the cache slot for the given frame index. */
    public [ForceInline] [mutating] void activate(const uint frame_index) {
        set_last_sampled(0u);
    }

    /* Accumulate into the cache slot. */
    public [ForceInline] [mutating] void accumulate(const float3 diffuse_sample, const float3 specular_sample) {
        /* Avoid accumulating too many samples, we can only handle ~1.000.000 */
        if (sample_count > 100000u) return;

        /* Convert to fixed point representation for atomics */
        const uint3 diffuse_fxp = uint3(diffuse_sample * QFACTOR);
        const uint3 specular_fxp = uint3(specular_sample * QFACTOR);

        /* Avoid atomics if the value is zero */
        if (diffuse_fxp.r > 0u) InterlockedAdd(diffuse.r, diffuse_fxp.r);
        if (diffuse_fxp.g > 0u) InterlockedAdd(diffuse.g, diffuse_fxp.g);
        if (diffuse_fxp.b > 0u) InterlockedAdd(diffuse.b, diffuse_fxp.b);
        if (specular_fxp.r > 0u) InterlockedAdd(specular.r, specular_fxp.r);
        if (specular_fxp.g > 0u) InterlockedAdd(specular.g, specular_fxp.g);
        if (specular_fxp.b > 0u) InterlockedAdd(specular.b, specular_fxp.b);
        InterlockedAdd(sample_count, 1u);
    }

    /* Reset the cache slot so it can be re-used. */
    public [ForceInline] [mutating] void reset() {
        key = EMPTY_KEY;
        diffuse = specular = 0u;
        sample_count = warmup_last = 0u;
    }
};

/* Insert data into the cache. */
public [ForceInline] void insert(RWStructuredBuffer<cache::Slot> cache, const uint64_t key, const uint frame_index, const float3 diffuse, const float3 specular) {
    /* Modulo the hash to fall within the cache capacity */
    const uint base_hash = voxel_hash(key) % CAPACITY;
    uint slot = base_hash;

    /* Find the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which already exists matching our key */
        if (cache[slot].key == key) {
            cache[slot].accumulate(diffuse, specular);
            return;
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % CAPACITY;
    }

    /* Find or insert the macrofacet */
    slot = base_hash;
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which already exists matching our key */
        if (cache[slot].key == key) {
            cache[slot].accumulate(diffuse, specular);
            return;
        }

        /* If the current slot is empty */
        if (cache[slot].key == EMPTY_KEY) {
            uint64_t original_key = 0u; /* Try to claim this slot */
            InterlockedCompareExchange(cache[slot].key, EMPTY_KEY, key, original_key);

            /* If instead this thread succeeded in claiming the slot */
            if (original_key == EMPTY_KEY || original_key == key) {
                cache[slot].accumulate(diffuse, specular);
                return;
            }
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % CAPACITY;
    }
}

/* Fetch a slot with a given key from the cache. */
public [ForceInline] Slot fetch(RWStructuredBuffer<cache::Slot> cache, const uint64_t key, const uint frame_index) {
    /* Modulo the hash to fall within the cache capacity */
    uint slot = voxel_hash(key) % CAPACITY;

    /* Find the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which matches our key */
        if (cache[slot].key == key) {
            cache[slot].activate(frame_index);
            return cache[slot];
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % CAPACITY;
    }

    return Slot(EMPTY_KEY); /* Couldn't find a sample */
}

/* Flush the spatial accumulator for a given key. */
public [ForceInline] void flush(RWStructuredBuffer<cache::Slot> cache, const uint64_t key) {
    /* Modulo the hash to fall within the cache capacity */
    uint slot = voxel_hash(key) % CAPACITY;

    /* Find the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which matches our key */
        if (cache[slot].key == key) {
            cache[slot].flush_accu();
            return;
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % CAPACITY;
    }
}

} // cache
