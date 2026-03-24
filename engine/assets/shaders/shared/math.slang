module math;

/* Large floating point constant. */
public static const float BIG_F32 = 1e30f;
/* PI constant. */
public static const float PI_F32 = 3.14159265359f;
public static const float RCP_PI_F32 = 1.0f / PI_F32;

public static const float GOLDEN_RATIO_F32 = 1.61803398875f;

/* Number of voxels per unit in world-space. */
public static const uint VOXELS_PER_UNIT = 10u;
/* Number of world-space units per voxel. */
public static const float UNITS_PER_VOXEL = 1.0f / VOXELS_PER_UNIT;

public struct Range {
    public float min;
    public float max;
};

// Bezier Helpers
float bezier_x(float t, float4 p) {
    const float u = 1.0f - t;
    return 3.0f * u * u * t * p.x + 3.0f * u * t * t * p.z + t * t * t;
}
float bezier_y(float t, float4 p) {
    const float u = 1.0f - t;
    return 3.0f * u * u * t * p.y + 3.0f * u * t * t * p.w + t * t * t;
}
float bezier_dx(float t, float4 p) {
    const float u = 1.0f - t;
    return 3.0f * u * u * p.x + 6.0f * u * t * (p.z - p.x) + 3.0f * t * t * (1.0f - p.z);
}

// Cubic Bezier
public struct BezierCurve {
    // 4 control points
    public float4 points;

    public float evaluate(float x) {
        if (is_linear())
            return x;

        // Newton's method: find t where bezier_x(t) == x
        float t = x; // initial guess
        for (int i = 0; i < 5; i++) {
            const float dx = bezier_dx(t, points);
            if (abs(dx) < 1e-6) break;
            t -= (bezier_x(t, points) - x) / dx;
            t = saturate(t);
        }
        return saturate(bezier_y(t, points));
    }

    public bool is_linear() {
        return points.x == 0.0f && points.y == 0.0f && points.z == 1.0f && points.w == 1.0f;
    }
};

/* Returns true if the given bit `i` in `x` is set. */
public [ForceInline] bool bit_set(const uint x, const uint i) {
    const uint mask = 1u << i;
    return (x & mask) != 0u;
}

/* Swap two variables around. */
public [ForceInline] void swap<T>(inout T a, inout T b) {
    const T c = a;
    a = b;
    b = c;
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

/* Decode a SDR color value packed as RGB10. */
public float3 decode_rgb10(const uint p) {
    const float rcp_scale = 1.0f / (float)(1u << 10);
    return float3(
        (float)(p & 0x3FFu) * rcp_scale,
        (float)((p >> 10) & 0x3FFu) * rcp_scale,
        (float)((p >> 20) & 0x3FFu) * rcp_scale
    );
}

/* Create orthonormal basis matrix for a given normal vector. */
public [ForceInline] float3x3 orthonormal_basis(const float3 n) {
    const float s = n.z >= 0.0f ? 1.0f : -1.0f;
    const float a = -1.0f / (s + n.z);
    const float b = n.x * n.y * a;
    /* Tangent, Bitangent, Normal */
    return float3x3(
        float3(1.0f + s * n.x * n.x * a, s * b, -s * n.x),
        float3(b, s + n.y * n.y * a, -n.y),
        n
    );
}

/* R1 quasi-random sequence. [0, 1) */
public float r1(const uint n) {
    /* Improved coefficients from <https://www.martysmods.com/a-better-r2-sequence/> */
    /* (1.0 - original coefficients) */
    return frac(n * 0.38196601125010515179366115080745f);
}

/* R1 quasi-random sequence. [0, 1) */
public float r1(inout float noise) {
    /* Improved coefficients from <https://www.martysmods.com/a-better-r2-sequence/> */
    /* (1.0 - original coefficients) */
    const float o = noise;
    noise = frac(o + 0.38196601125010515179366115080745f);
    return o;
}

/* R1 quasi-random sequence. [0, 1) */
public float r1(const float noise, const uint n) {
    /* Improved coefficients from <https://www.martysmods.com/a-better-r2-sequence/> */
    /* (1.0 - original coefficients) */
    return frac(noise + float(n) * 0.38196601125010515179366115080745f);
}

/* R2 quasi-random sequence. [0, 1) */
public float2 r2(const uint n) {
    /* Improved coefficients from <https://www.martysmods.com/a-better-r2-sequence/> */
    /* (1.0 - original coefficients) */
    return frac(n * float2(0.2451223337533073f, 0.4301597090019468f));
}

/* R2 quasi-random sequence. [0, 1) */
public float2 r2(const float2 noise, const uint i) {
    /* Improved coefficients from <https://www.martysmods.com/a-better-r2-sequence/> */
    /* (1.0 - original coefficients) */
    return frac(noise + float(i) * float2(0.2451223337533073f, 0.4301597090019468f));
}

uint pcg(inout uint state) {
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

/* Random float in range [0, 1) */
public float rand_float(inout uint state) {
    return pcg(state) / 4294967296.0f;
}

/* Random float2 in range [0, 1) */
public float2 rand_float2(inout uint state) {
    return float2(rand_float(state), rand_float(state));
}

/* Generate a sample direction within a cone around a direction vector. */
public float3 sample_cone(const float3 dir, const float half_angle, const float2 uv) {
    const float cos_theta = 1.0f - uv.x * (1.0f - half_angle);
    const float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    const float phi = 2.0f * PI_F32 * uv.y;
    const float3 local_dir = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
    const float3x3 basis = orthonormal_basis(dir);
    return normalize(local_dir.x * basis[0] + local_dir.y * basis[1] + local_dir.z * dir);
}

/* Generate a cosine weighted sample in the hemisphere around a normal vector. */
public float3 sample_hemisphere_cosine(const float3 n, const float2 uv) {
    const float r = sqrt(uv.x);
    const float phi = 2.0f * PI_F32 * uv.y;
    const float x = r * cos(phi);
    const float y = r * sin(phi);
    const float z = sqrt(1.0f - uv.x);
    const float3x3 basis = orthonormal_basis(n);
    return normalize(x * basis[0] + y * basis[1] + z * n);
}

/* Generate a uniformly weighted sample in the hemisphere around a normal vector. */
public float3 sample_hemisphere_uniform(const float3 n, const float2 uv) {
    const float z = uv.x;
    const float r = sqrt(1.0f - z * z);
    const float phi = 2.0f * PI_F32 * uv.y;
    const float x = r * cos(phi);
    const float y = r * sin(phi);
    const float3x3 basis = orthonormal_basis(n);
    return normalize(x * basis[0] + y * basis[1] + z * n);
}

/* Convert a 3D direction to a lat long coordinate. */
public inline float2 dir_to_latlong(const float3 dir) {
    const float phi = atan2(dir.x, dir.z);
    const float theta = acos(dir.y);
    const float2 uv = float2(phi / (PI_F32 * 2.0f), theta / PI_F32);
    return uv;
}
