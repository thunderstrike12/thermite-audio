module colorspace;

namespace cs {

/* Color-space transformation matrices (generated using https://github.com/mxcop/chroma) */
static const float3x3 R709_TO_ACESCG = { 0.613097f, 0.070194f, 0.020616f, 0.339523f, 0.916354f, 0.109570f, 0.047380f, 0.013452f, 0.869815f };
static const float3x3 ACESCG_TO_R709 = { 1.705051f, -0.130256f, -0.024003f, -0.621792f, 1.140805f, -0.128969f, -0.083259f, -0.010548f, 1.152972f };
static const float3x3 ACESCG_TO_ACES2065 = { 0.695452f, 0.044795f, -0.005526f, 0.140679f, 0.859671f, 0.004025f, 0.163869f, 0.095534f, 1.001501f };
static const float3x3 ACES2065_TO_ACESCG = { 1.451439f, -0.076554f, 0.008316f, -0.236511f, 1.176230f, -0.006032f, -0.214929f, -0.099676f, 0.997716f };
static const float3x3 XYZ_TO_ACESCG = { 1.641023f, -0.663663f, 0.011722f, -0.324803f, 1.615332f, -0.008284f, -0.236425f, 0.016756f, 0.988395f };

public [ForceInline] float linearize(const float value) {
    if (value <= 0.04045f) return value / 12.92f;
    return pow((value + 0.055f) / 1.055f, 2.4f);
}

public [ForceInline] float3 linearize(const float3 value) {
    return float3(linearize(value.r), linearize(value.g), linearize(value.b));
}

public [ForceInline] float delinearize(const float value) {
    if (value <= 0.018f) return value * 4.5f;
    return 1.099f * pow(value, 0.45f) - 0.099f;
}

public [ForceInline] float3 delinearize(const float3 value) {
    return float3(delinearize(value.r), delinearize(value.g), delinearize(value.b));
}

public [ForceInline] float3 aces_filmic_tonemapping(const float3 acescg) {
    /* Apply RRT and ODT fitting */
    /* Source: <https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl#L30> */
    const float3 a = acescg * (acescg + 0.0245786f) - 0.000090537f;
    const float3 b = acescg * (0.983729f * acescg + 0.4329510f) + 0.238081f;
    const float3 fitted = a / b;
    return saturate(fitted);
}

/* Rec.709 / ACEScg transformations */
public [ForceInline] float3 r709_to_acescg(const float3 r709) { return mul(r709, R709_TO_ACESCG); }
public [ForceInline] float3 acescg_to_r709(const float3 acescg) { return saturate(mul(acescg, ACESCG_TO_R709)); }

/* ACEScg / ACES2065-1 transformations */
public [ForceInline] float3 acescg_to_aces2065(const float3 acescg) { return mul(acescg, ACESCG_TO_ACES2065); }
public [ForceInline] float3 aces2065_to_acescg(const float3 aces2065) { return saturate(mul(aces2065, ACES2065_TO_ACESCG)); }

/* Get the luminance of a linear ACEScg color. */
public [ForceInline] float acescg_luminance(const float3 acescg) {
    return dot(acescg, float3(0.2722287168f, 0.6740817658f, 0.0536895174f));
}

}
