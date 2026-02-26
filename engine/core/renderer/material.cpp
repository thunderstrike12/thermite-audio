#include "material.hpp"

#include <glm/ext/scalar_constants.hpp>
#include <graphite/vram_bank.hh>

#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

/* Compute directional albedo integral. */
inline float compute_e(float wo_costheta, float roughness, uint32_t sample_count = 1024u) {
    const float alpha = roughness * roughness;
    const float alpha_2 = alpha * alpha;
    
    /* Build the view vector in tangent space (N = 0,0,1) */
    const float wo_sintheta = 1.0f - wo_costheta;
    const glm::vec3 wo = glm::vec3(wo_sintheta, 0.0f, wo_costheta);
    
    float e = 0.0f;
    for (uint32_t i = 0u; i < sample_count; ++i) {
        /* Compute 2D hammersley sequence */
        const float u1 = (float)i / (float)sample_count;
        uint32_t bits = i;
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        const float u2 = (float)bits * 2.3283064365386963e-10f; /* 0x100000000 */
        
        /* Sample GGX NDF */
        const float phi = 2.0f * glm::pi<float>() * u1;
        const float wh_costheta = sqrt((1.0f - u2) / (1.0f + (alpha_2 - 1.0f) * u2));
        const float wh_sintheta = sqrt(1.0f - wh_costheta * wh_costheta);
        const glm::vec3 wh = glm::vec3(wh_sintheta * glm::cos(phi), wh_sintheta * glm::sin(phi), wh_costheta);
        
        /* Reflect outgoing around half-way to get incident */
        const glm::vec3 wi = 2.0f * glm::dot(wo, wh) * wh - wo;
        const float wi_costheta = wi.z;
        
        if (wi_costheta > 0.0f) {
            /* Smith G2 height correlated for GGX */
            const float g_v = 2.0f * wo_costheta / (wo_costheta + glm::sqrt(alpha_2 + (1.0f - alpha_2) * wo_costheta * wo_costheta));
            const float g_l = 2.0f * wi_costheta / (wi_costheta + glm::sqrt(alpha_2 + (1.0f - alpha_2) * wi_costheta * wi_costheta));
            const float g = g_v * g_l;

            /* Bunch of terms cancel out thanks to not caring about fresnel and importance sampling GGX */
            e += g * glm::max(glm::dot(wo, wh), 0.0f) / (wo_costheta * wh_costheta);
        }
    }
    
    return e / (float)sample_count;
}

void generate_e_lut(Texture& out_texture, const uint32_t resolution) {
    /* Allocate space for the LUT */
    float* lut = new float[resolution * resolution];

    /* Compute the LUT */
    const float step = 1.0f / (float)resolution;
    for (uint32_t y = 0u; y < resolution; ++y) {
        const float roughness = y * step + step * 0.5f;
        for (uint32_t x = 0u; x < resolution; ++x) {
            const float wo_costheta = x * step + step * 0.5f;
            lut[x + y * resolution] = 1.0f - compute_e(wo_costheta, roughness);
        }
    }

    /* Create and upload look-up texture resource */
    VRAMBank& bank = engine.renderer.vram_bank();
    out_texture = bank.create_texture("Directional Albedo LUT Texture", TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::R32Sfloat, Size3D(resolution, resolution)).expect("failed to create directional albedo lut texture.");
    bank.upload_texture(out_texture, lut, resolution * resolution * sizeof(float));

    /* Free the LUT */
    delete[] lut;
}

}  // namespace tmt
