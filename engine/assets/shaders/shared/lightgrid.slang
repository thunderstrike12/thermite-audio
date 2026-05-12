module lightgrid;

import math;
import lights;

namespace lgrid {

/* Size of one cell in words. */
public static const uint CELL_STRIDE = 64u;
/* Maximum number of lights per cell. (NOTE: first word is used as light count) */
public static const uint CELL_MAX_LIGHTS = CELL_STRIDE - 1u;

/* Total maximum number of lights per scene. */
public static const uint MAX_LIGHT_COUNT = 1024u;
public static const uint WORD_COUNT = MAX_LIGHT_COUNT >> 5; /* max_lights / 32 bits */

public static constexpr uint32_t CASCADE_COUNT = 6u;
public static constexpr uint32_t FIRST_CASCADE_BOUNDS = 32u; /* u */
public static constexpr uint32_t CASCADE_RESOLUTION = 16u; /* cells */
public static constexpr uint32_t CASCADE_RESOLUTION2 = CASCADE_RESOLUTION * CASCADE_RESOLUTION;
public static constexpr uint32_t CASCADE_RESOLUTION3 = CASCADE_RESOLUTION * CASCADE_RESOLUTION * CASCADE_RESOLUTION;

public static constexpr float CASCADE_LOG_OFFSET = log2((float)FIRST_CASCADE_BOUNDS) - 1.0f; /* log2(half_extent) = log2(32) - 1 = 4 */
public static constexpr float GRID_HALF_RES = (float)CASCADE_RESOLUTION / 2.0f; /* 16 / 2 = 8 */

public struct CascadeIndex {
    public uint32_t cascade = 0u;
    public uint32_t x = 0u;
    public uint32_t y = 0u;
    public uint32_t z = 0u;
};

/* Index light cascades using view-space point `p`. */
public inline CascadeIndex index_cascades(const float3 p) {
    /* Calculate the cascade index */
    const float max_axis = max(max(abs(p.x), abs(p.y)), abs(p.z));
    const uint32_t cascade_index = (uint32_t)max(0.0f, ceil(log2(max_axis) - CASCADE_LOG_OFFSET));
    if (cascade_index >= CASCADE_COUNT)
        return { uint::maxValue, 0u, 0u, 0u }; /* Out of bounds */

    /* Find which cell in the cascade the local point falls into */
    const uint32_t cascade_cell_extent = 1u << (cascade_index + 1u);
    const float3 cascade_point = floor(p / (float)cascade_cell_extent) + GRID_HALF_RES;
    return { cascade_index, (uint32_t)cascade_point.x, (uint32_t)cascade_point.y, (uint32_t)cascade_point.z };
}

/* Get the base index of a cell in the light grid buffer. */
public [ForceInline] uint cell_base(const uint cascade, const uint x, const uint y, const uint z) {
    return ((cascade * CASCADE_RESOLUTION3) + (z * CASCADE_RESOLUTION2) + (y * CASCADE_RESOLUTION) + x) * CELL_STRIDE;
}

/* Get the number of lights overlapping a given view-space point. */
public [ForceInline] uint light_occupancy(const StructuredBuffer<uint32_t> cascades, const float3 pos) {
    /* Find the cascade index */
    const CascadeIndex i = index_cascades(pos);
    if (i.cascade == uint::maxValue || i.cascade >= lgrid::CASCADE_COUNT) return 0u;

    /* Calculate the cell index and read the count from the cascades */
    return cascades[cell_base(i.cascade, i.x, i.y, i.z)];
}

public struct PickedLight {
    public uint index;    /* Selected light index. */
    public float rcp_pdf; /* Reciprocal probability distribution function. */
};

/* Pick a light overlapping a given view-space point. */
public [ForceInline] PickedLight pick_light(const StructuredBuffer<uint32_t> cascades, const float3 pos, inout uint seed) {
    /* Index light grid cascades */
    const lgrid::CascadeIndex index = lgrid::index_cascades(pos);
    if (index.cascade == uint::maxValue) return PickedLight(uint::maxValue, BIG_F32);

    /* Compute cell index and base, and retrieve light count */
    const uint32_t cell_base = lgrid::cell_base(index.cascade, index.x, index.y, index.z);
    const uint32_t light_count = cascades[cell_base];
    if (light_count == 0u) return PickedLight(uint::maxValue, BIG_F32);

    /* Pick a random light uniformly */
    const uint32_t slot = uint32_t(rand_float(seed) * light_count);
    const uint32_t light_index = cascades[cell_base + 1u + slot];
    return PickedLight(light_index, (float)light_count);
}

public [ForceInline] LightSample sample_light(const StructuredBuffer<uint32_t> cascades, StructuredBuffer<Light> lights, const float3 pos, const float3 view, inout uint seed, const float2 noise) {
    /* Index light grid cascades */
    const lgrid::CascadeIndex index = lgrid::index_cascades(pos - view);
    if (index.cascade == uint::maxValue) return LightSample(0.0f, 0.0f, 0.0f);

    /* Compute cell index and base, and retrieve light count */
    const uint32_t cell_base = lgrid::cell_base(index.cascade, index.x, index.y, index.z);
    const uint32_t light_count = cascades[cell_base];
    if (light_count == 0u) return LightSample(0.0f, 0.0f, 0.0f);

    /* Pick a random light uniformly */
    const uint32_t slot = uint32_t(rand_float(seed) * light_count);
    const uint32_t light_index = cascades[cell_base + 1u + slot];

    /* Generate a sample for the light we picked */
    LightSample s = sample_light(pos, lights[light_index], noise);
    s.index = light_index;
    s.rcp_pdf *= light_count; /* Update PDF */
    return s;
}

}
