/**
 * @name Thermite Material Model.
 * This file contains the implementation for our material model based on OpenPBR.
 * Our shading frame is Z-up.
 */
module material;

import math;

public struct ShadingFrame {
    public float3 wi;
    public float3 wo;
    public float3 wh;
};

/* Create a shading frame for the given normal, incident and outgoing directions. */
public [ForceInline] ShadingFrame create_shading_frame(const float3 n, const float3 wi, const float3 wo) {
    const Basis basis = orthonormal_basis(n);
    const float3x3 world_to_shade = float3x3(basis.a, basis.b, n);
    const float3 wi_shade = mul(world_to_shade, wi);
    const float3 wo_shade = mul(world_to_shade, wo);
    return ShadingFrame(wi_shade, wo_shade, normalize(wi_shade + wo_shade));
}

/* Voxel material data, used in a palette. */
public struct Material {
    public uint packed_albedo; /* Packed 10bit RGB color. */
    public uint ior_emission; /* Packed f16 ior, f16 emission. */
    public uint roughness_metallic_transmission; /* Packed 8bit roughness, metallic, and transmission. (1 byte unused) */

    /* Material base color (ACEScg) */
    public [ForceInline] float3 albedo() { return decode_rgb10(packed_albedo); };
    /* Material index of refraction [1,3] */
    public [ForceInline] float ior() { return unpackHalf2x16ToFloat(ior_emission).x; };
    /* Material microfacet roughness [0,1] */
    public [ForceInline] float roughness() { return float(roughness_metallic_transmission & 0xFFu) * (1.0f / 0xFFu); };
};

/* f_layer(ωi,ωo) = f_coat(ωi,ωo) + (1 - E_coat(ωo)) * f_sub(ωi,ωo) */
/* <https://academysoftwarefoundation.github.io/OpenPBR/#layering> */
[ForceInline] float3 f_layer(const float3 fsub, const float3 fcoat) {
    /* Do we need "directional albedo integral" here? (7) */
    /* <https://pbr-book.org/4ed/Reflection_Models/BSDF_Representation#HemisphericalReflectance> */
    return fcoat + fsub;
}

/* f_mix = (1 - w1) * f0 + w1 * f1 = lerp(f0,f1,w1) */
/* <https://academysoftwarefoundation.github.io/OpenPBR/#mixing> */
[ForceInline] float3 f_mix(const float3 f0, const float3 f1, const float w1) {
    return (1.0f - w1) * f0 + w1 * f1;
}

/*******************************************************************/
/*                                                                 */
/*  ~ thermite material model                        emission      */
/*                                                      ^          */
/*    ambient medium                                    |          */
/*  +---------------------+---------------------+-------+-------+  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |               |  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |     gloss     |  */
/*  |░░░░░░░ metal ░░░░░░░|     translucent     |               |  */
/*  |░░░░░░░░░░░░░░░░░░░░░|        base         +---------------+  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |░░░░░░░░░░░░░░░|  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |░░░░diffuse░░░░|  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |░░░░░░░░░░░░░░░|  */
/*  + - - - - - - - - - - +- - - - - - - - - - -+ - - - - - - - +  */
/*                                              <- opaque-base ->  */
/*                        <---------- dielectric-base ---------->  */
/*  <--------------------------- base -------------------------->  */
/*                                                                 */
/*******************************************************************/

/* Lambertian BRDF. */
/* Source: https://pbr-book.org/4ed/Reflection_Models */
[ForceInline] float3 diffuse_slab(const float3 albedo) {
    return albedo * RCP_PI_F32;
}

/* Torrance–Sparrow Microfacet BRDF. */
/* Source: https://www.pbr-book.org/4ed/Reflection_Models */ 
[ForceInline] float gloss_slab(
    const float wi_costheta, /* incident cos(theta) */
    const float wo_costheta, /* outgoing cos(theta) */
    const float wh_costheta, /* half-way cos(theta) */
    const float alpha_2, float eta
) {
    /* Trowbridge-Reitz normal distribution function */
    const float wh_costheta_2 = wh_costheta * wh_costheta;
    const float wh_costheta_4 = wh_costheta_2 * wh_costheta_2;
    const float wh_sintheta_2 = 1.0f - wh_costheta_2;
    const float wh_tantheta_2 = wh_sintheta_2 / wh_costheta_2;
    const float e = alpha_2 + wh_tantheta_2;
    const float d = alpha_2 / (PI_F32 * wh_costheta_4 * e * e);

    /* Trowbridge-Reitz masking and shadowing function */
    const float wi_sintheta_2 = 1.0f - wi_costheta * wi_costheta;
    const float wi_tantheta_2 = wi_sintheta_2 / (wi_costheta * wi_costheta);
    const float wi_lambda = (-1.0f + sqrt(1.0f + alpha_2 * wi_tantheta_2)) * 0.5f;
    const float wo_sintheta_2 = 1.0f - wo_costheta * wo_costheta;
    const float wo_tantheta_2 = wo_sintheta_2 / (wo_costheta * wo_costheta);
    const float wo_lambda = (-1.0f + sqrt(1.0f + alpha_2 * wo_tantheta_2)) * 0.5f;
    const float g = 1.0f / (1.0f + wi_lambda + wo_lambda);

    float i_costheta = wi_costheta;
    /* If the incoming direction comes from within */
    if (i_costheta < 0.0f) {
        /* Invert relative index of refraction, ensuring cos(theta_i) is always positive */
        eta = rcp(eta);
        i_costheta = -i_costheta;
    }

    /* Apply Snell's law to find cos(theta_t) */
    const float i_sintheta_2 = 1.0f - i_costheta * i_costheta;
    const float t_sintheta_2 = i_sintheta_2 / (eta * eta);
    const bool t_int = t_sintheta_2 >= 1.0f; /* Total internal reflection */
    const float t_costheta = sqrt(1.0f - t_sintheta_2);

    /* Fresnel equation */
    const float r_parl = (eta * i_costheta - t_costheta) / (eta * i_costheta + t_costheta);
    const float r_perp = (i_costheta - eta * t_costheta) / (i_costheta + eta * t_costheta);
    const float f = t_int ? 1.0f : ((r_parl * r_parl + r_perp * r_perp) * 0.5f);

    /* Torrance–Sparrow BRDF */
    return (d * g * f) / (4.0f * wo_costheta * wi_costheta);
}

/* Evaluate opaque base material slab. (diffuse & gloss) */
[ForceInline] public float3 opaque_base_slab(
    const float wi_costheta, const float wo_costheta, 
    const float wh_costheta, const Material mat,
) {
    /* Evaluate the two slabs */
    const float3 diffuse = 1.0f; // diffuse_slab(mat.albedo());
    const float roughness = mat.roughness();
    const float alpha = roughness * roughness;
    const float eta = mat.ior() / 1.0f;
    const float3 gloss = gloss_slab(wi_costheta, wo_costheta, wh_costheta, alpha * alpha, eta);

    /* Layer the two slabs, diffuse = substrate, gloss = coating */
    return f_layer(diffuse, gloss);
}
