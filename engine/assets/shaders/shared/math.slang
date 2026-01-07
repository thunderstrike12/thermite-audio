module math;

/* Large floating point constant. */
public static const float BIG_F32 = 1e30f;

/* Number of voxels per unit in world-space. */
public static const uint VOXELS_PER_UNIT = 10u;
/* Number of world-space units per voxel. */
public static const float UNITS_PER_VOXEL = 1.0f / VOXELS_PER_UNIT;

/* Returns true if the given bit `i` in `x` is set. */
public [ForceInline] bool bit_set(const uint x, const uint i) {
    const uint mask = 1u << i;
    return (x & mask) != 0u;
}

/* Convert a uint64 into two uint32s. */
public [ForceInline] uint2 asuint2(const uint64_t i) {
    return uint2((uint)i, (uint)(i >> 32));
}

/* Convert two uint32s into a single uint64. */
public [ForceInline] uint64_t asuint64(const uint2 i) {
    return ((uint64_t)i.y << 32) | i.x;
}

/* Encode a HDR color value packed as RGB9E5. */
public uint encode_rgb9e5(const float3 rgb) {
    const float kmax = 65408.0f; /* 1.FF x 2^+15 */
    const float kmin = .000016f; /* 1.00 x 2^-16 */

    /* Clamp the RGB values */
    const float sr = clamp(rgb.x, kmin, kmax);
    const float sg = clamp(rgb.y, kmin, kmax);
    const float sb = clamp(rgb.z, kmin, kmax);

    /* Find the maximum channel */
    const float maxc = max(max(kmin, sr), max(sg, sb));
    const uint bias = (asuint(maxc) + 0x07804000u) & 0x7F800000u;
    const float fbias = asfloat(bias);

    /* Finally shift the bits into their places */
    const float pr = sr + fbias;
    const float pg = sg + fbias;
    const float pb = sb + fbias;
    const uint pe = (bias << 4u) + 0x10000000u;
    return pe | (asuint(pb) << 18u) | (asuint(pg) << 9u) | (asuint(pr) & 0x1FFu);
}

/* Decode a HDR color value packed as RGB9E5. */
public float3 decode_rgb9e5(const uint p) {
    const float x = p & 0x1FFu, y = (p >> 9u) & 0x1FFu, z = (p >> 18u) & 0x1FFu;
    const int e = (int)(p >> 27u) - 24;
    const float r = ldexp(x, e), g = ldexp(y, e), b = ldexp(z, e);
    return float3(r, g, b);
}
