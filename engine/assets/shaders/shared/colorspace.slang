module colorspace;

namespace cs {

/* Color-space transformation matrices (generated using https://github.com/mxcop/chroma) */
static const float3x3 R709_TO_ACESCG = { 0.597314f, 0.073732f, 0.020689f, 0.332820f, 0.917381f, 0.118816f, 0.038138f, 0.017072f, 0.961536f };
static const float3x3 ACESCG_TO_R709 = { 1.753791f, -0.140577f, -0.020365f, -0.628701f, 1.142966f, -0.127707f, -0.058399f, -0.014717f, 1.043079f };
static const float3x3 ACESCG_TO_ACES2065 = { 0.695452f, 0.044795f, -0.005526f, 0.140679f, 0.859671f, 0.004025f, 0.163869f, 0.095534f, 1.001501f };
static const float3x3 ACES2065_TO_ACESCG = { 1.451439f, -0.076554f, 0.008316f, -0.236511f, 1.176230f, -0.006033f, -0.214929f, -0.099676f, 0.997716f };
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
public [ForceInline] float3 acescg_to_r709(const float3 acescg) { return max(mul(acescg, ACESCG_TO_R709), 0.0f); }

/* ACEScg / ACES2065-1 transformations */
public [ForceInline] float3 acescg_to_aces2065(const float3 acescg) { return mul(acescg, ACESCG_TO_ACES2065); }
public [ForceInline] float3 aces2065_to_acescg(const float3 aces2065) { return max(mul(aces2065, ACES2065_TO_ACESCG), 0.0f); }

/* Get the luminance of a linear ACEScg color. */
public [ForceInline] float acescg_luminance(const float3 acescg) {
    return dot(acescg, float3(0.2722287168f, 0.6740817658f, 0.0536895174f));
}

}
