#include "colorspace.hpp"

namespace tmt::cs {

/* clang-format off */

/* Color-space transformation matrices (generated using https://github.com/mxcop/chroma) */
constexpr glm::mat3 R709_TO_ACESCG { 0.597314f, 0.073732f, 0.020689f, 0.332820f, 0.917381f, 0.118816f, 0.038138f, 0.017072f, 0.961536f };
constexpr glm::mat3 ACESCG_TO_R709 { 1.753791f, -0.140577f, -0.020365f, -0.628701f, 1.142966f, -0.127707f, -0.058399f, -0.014717f, 1.043079f };
constexpr glm::mat3 ACESCG_TO_ACES2065 { 0.695452f, 0.044795f, -0.005526f, 0.140679f, 0.859671f, 0.004025f, 0.163869f, 0.095534f, 1.001501f };
constexpr glm::mat3 ACES2065_TO_ACESCG { 1.451439f, -0.076554f, 0.008316f, -0.236511f, 1.176230f, -0.006033f, -0.214929f, -0.099676f, 0.997716f };
constexpr glm::mat3 XYZ_TO_ACESCG { 1.641023f, -0.663663f, 0.011722f, -0.324803f, 1.615332f, -0.008284f, -0.236425f, 0.016756f, 0.988395f };

float linearize(const float value) {
    if (value <= 0.04045f) return value / 12.92f;
    return powf((value + 0.055f) / 1.055f, 2.4f);
}

glm::vec3 linearize(const glm::vec3 value) {
    return glm::vec3(linearize(value.r), linearize(value.g), linearize(value.b));
}

float delinearize(const float value) {
    if (value <= 0.018f) return value * 4.5f;
    return 1.099f * powf(value, 0.45f) - 0.099f;
}

glm::vec3 delinearize(const glm::vec3 value) {
    return glm::vec3(delinearize(value.r), delinearize(value.g), delinearize(value.b));
}

glm::vec3 aces_filmic_tonemapping(const glm::vec3 acescg) {
    /* Apply RRT and ODT fitting */
    /* Source: <https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl#L30> */
    const glm::vec3 a = acescg * (acescg + 0.0245786f) - 0.000090537f;
    const glm::vec3 b = acescg * (0.983729f * acescg + 0.4329510f) + 0.238081f;
    const glm::vec3 fitted = a / b;
    return glm::clamp(fitted, 0.0f, 1.0f);
}

/* Rec.709 / ACEScg transformations */
glm::vec3 r709_to_acescg(const glm::vec3 r709) { return R709_TO_ACESCG * r709; }
glm::vec3 acescg_to_r709(const glm::vec3 acescg) { return glm::max(ACESCG_TO_R709 * acescg, 0.0f); }

/* ACEScg / ACES2065-1 transformations */
glm::vec3 acescg_to_aces2065(const glm::vec3 acescg) { return ACESCG_TO_ACES2065 * acescg; }
glm::vec3 aces2065_to_acescg(const glm::vec3 aces2065) { return glm::max(ACES2065_TO_ACESCG * aces2065, 0.0f); }

/* Calculate the black body color for a given temperature in Kelvin in the linear ACEScg color-space. */
/* Equations from <https://google.github.io/filament/Filament.html#lighting> */
glm::vec3 black_body_acescg(const float k) {
    const float k2 = k * k;

    const float u_num = 0.860117757f + 1.54118254e-4f * k + 1.28641212e-7f * k2;
    const float u_den = 1.0f + 8.42420235e-4f * k + 7.08145163e-7f * k2;
    const float u = u_num / u_den;

    const float v_num = 0.317398726f + 4.22806245e-5f * k + 4.20481691e-8f * k2;
    const float v_den = 1.0f - 2.89741816e-5f * k + 1.61456053e-7f * k2;
    const float v = v_num / v_den;

    const float xy_den = 2.0f * u - 8.0f * v + 4.0f;
    const float x = (3.0f * u) / xy_den;
    const float y = (2.0f * v) / xy_den;

    const float cx = x / y;
    const float cz = (1.0f - x - y) / y;

    /* Convert from CIE XYZ to ACEScg color-space and normalize the result */
    const glm::vec3 acescg = XYZ_TO_ACESCG * glm::vec3(cx, 1.0f, cz);
    return acescg; /* / glm::max(acescg.x, glm::max(acescg.y, acescg.z)) */
}

/* clang-format on */

}  // namespace tmt::cs
