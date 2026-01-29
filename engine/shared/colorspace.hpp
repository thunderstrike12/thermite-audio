#pragma once

namespace tmt::cs {

/* Transform color from non-linear to linear space. */
float linearize(const float value);
/* Transform color from non-linear to linear space. */
glm::vec3 linearize(const glm::vec3 value);

/* Transform color from linear to non-linear space. */
float delinearize(const float value);
/* Transform color from linear to non-linear space. */
glm::vec3 delinearize(const glm::vec3 value);

/* ACES tonemapping curve (Narkowicz 2015) */
/* Expects ACEScg input, returns ACEScg output. */
glm::vec3 aces_filmic_tonemapping(const glm::vec3 acescg);

/* Transform color from linear SRGB (Rec.709) to linear ACEScg color-space. */
glm::vec3 r709_to_acescg(const glm::vec3 linear_srgb);
/* Transform color from linear ACEScg to linear SRGB (Rec.709) color-space. */
glm::vec3 acescg_to_r709(const glm::vec3 acescg);

/* Transform color from linear ACEScg to linear ACES2065-1 color-space. */
glm::vec3 acescg_to_aces2065(const glm::vec3 srgb);
/* Transform color from linear ACES2065-1 to linear ACEScg color-space. */
glm::vec3 aces2065_to_acescg(const glm::vec3 acescg);

/* Calculate the black body color for a given temperature in Kelvin in the linear ACEScg color-space. */
glm::vec3 black_body_acescg(const float k);

}  // namespace tmt::cs
