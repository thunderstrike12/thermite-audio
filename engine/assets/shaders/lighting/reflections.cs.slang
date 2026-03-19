import view;
import math;
import visibility;
import voxel;
import bvh2;
import cache;
import lights;
import material;
import bindless;

/* Active render & scene view. */
ConstantBuffer<RenderView> render_view;
ConstantBuffer<SceneView> scene_view;
/* Environment map sampler. */
SamplerState envmap_sampler;
Texture2D<float4> blue_noise;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Macrofacet cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Luminance buffer. */
RWTexture2D<float3> specular;

/* Sample glossy specular lobe using VNDF distribution. */
/* Source: "Bounded VNDF Sampling for Smith–GGX Reflections" */
[ForceInline] float3 sample_vndf_bounded(float3 i, float alpha, float alpha_2, float2 r01) {
    /* Sample cap */
    const float3 i_std = normalize(float3(i.xy * alpha, i.z));
    const float phi = 2.0f * PI_F32 * r01.x;
    const float s = 1.0f + length(float2(i.x, i.y));
    const float s2 = s * s;
    const float k = (1.0f - alpha_2) * s2 / (s2 + alpha_2 * i.z * i.z);
    const float b = i.z > 0.0f ? k * i_std.z : i_std.z;
    const float z = mad(1.0f - r01.y, 1.0f + b, -b);
    const float sin_theta = sqrt(saturate(1.0f - z * z));
    const float3 o_std = float3(sin_theta * cos(phi), sin_theta * sin(phi), z);
    const float3 m_std = i_std + o_std;
    return normalize(float3(m_std.xy * alpha, m_std.z));
}

// /* PDF for bounded glossy specular lobe using VNDF distribution. */
// /* Source: "Bounded VNDF Sampling for Smith–GGX Reflections" */
// float pdf_vndf_bounded(float3 wi, float wh_costheta, float alpha, float alpha_2) {
//     /* Trowbridge-Reitz normal distribution function */
//     const float wh_costheta_2 = wh_costheta * wh_costheta;
//     const float wh_costheta_4 = wh_costheta_2 * wh_costheta_2;
//     const float wh_sintheta_2 = 1.0f - wh_costheta_2;
//     const float wh_tantheta_2 = wh_sintheta_2 / wh_costheta_2;
//     const float e = alpha_2 + wh_tantheta_2;
//     const float d = alpha_2 / (PI_F32 * wh_costheta_4 * e * e);
//     /* Sample cap */
//     const float2 ai = alpha * wi.xy;
//     const float len2 = dot(ai, ai);
//     const float t = sqrt(len2 + wi.z * wi.z);
//     if (wi.z >= 0.0f) {
//         const float s = 1.0f + length(float2(wi.x, wi.y));
//         const float s2 = s * s;
//         const float k = (1.0f - alpha_2) * s2 / (s2 + alpha_2 * wi.z * wi.z);
//         return d / (2.0f * (k * wi.z + t));
//     }
//     return d * (t - wi.z) / (2.0f * len2);
// }

// float3 SampleBoundedVNDF_Isotropic(float3 i, float alpha, float2 rand) {
//     // Stretch incoming direction to std space
//     float3 i_std = normalize(float3(i.xy * alpha, i.z));

//     // Bounded spherical cap (Eq. 5 & 6 from the paper)
//     float s = 1.0f + length(i.xy);  // sgn omitted since alpha <= 1
//     float a2 = alpha * alpha;
//     float s2 = s * s;
//     float k = (1.0f - a2) * s2 / (s2 + a2 * i.z * i.z);

//     // Lower bound: use k*i_std.z when front-facing, else fallback
//     float b = (i.z > 0) ? k * i_std.z : i_std.z;

//     // Sample spherical cap in (-b, 1]
//     float phi = 2.0f * PI_F32 * rand.x;
//     float z = mad(1.0f - rand.y, 1.0f + b, -b);
//     float sinTheta = sqrt(saturate(1.0f - z * z));
//     float3 o_std = float3(sinTheta * cos(phi), sinTheta * sin(phi), z);

//     // Halfvector → microfacet normal → unstretched
//     float3 m_std = i_std + o_std;
//     return normalize(float3(m_std.xy * alpha, m_std.z));

//     // Reflection
//     // return 2.0f * dot(i, m) * m - i;
// }

// float BoundedVNDF_PDF_Isotropic(float3 i, float3 o, float alpha) {
//     float3 m = normalize(i + o);
//     float a2 = alpha * alpha;

//     /* Trowbridge-Reitz normal distribution function */
//     const float wh_costheta_2 = m.z * m.z;
//     const float wh_costheta_4 = wh_costheta_2 * wh_costheta_2;
//     const float wh_sintheta_2 = 1.0f - wh_costheta_2;
//     const float wh_tantheta_2 = wh_sintheta_2 / wh_costheta_2;
//     const float e = a2 + wh_tantheta_2;
//     const float ndf = a2 / (PI_F32 * wh_costheta_4 * e * e);

//     float len2 = a2 * dot(i.xy, i.xy);  // |alpha * i.xy|^2
//     float t = sqrt(len2 + i.z * i.z);

//     if (i.z >= 0.0f)
//     {
//         float s = 1.0f + length(i.xy);
//         float s2 = s * s;
//         float k = (1.0f - a2) * s2 / (s2 + a2 * i.z * i.z);
//         return ndf / (2.0f * (k * i.z + t));  // Eq. 8 * ||dm/do||
//     }

//     // Backfacing: standard unbounded PDF
//     return ndf * (t - i.z) / (2.0f * len2);
// }

// float3 sample_vndf_unbounded(float3 i, float alpha, float2 rand) {
//     float3 i_std = normalize(float3(i.xy * alpha, i.z));

//     float phi = 2.0f * PI_F32 * rand.x;
//     float z = fma(1.0f - rand.y, 1.0f + i_std.z, -i_std.z);
//     float sinTheta = sqrt(saturate(1.0f - z * z));
//     float3 o_std = float3(sinTheta * cos(phi), sinTheta * sin(phi), z);

//     float3 m_std = i_std + o_std;
//     return normalize(float3(m_std.xy * alpha, m_std.z));
// }

// float pdf_vndf_unbounded(float3 i, float3 o, float alpha) {
//     float3 m = normalize(i + o);
//     float a2 = alpha * alpha;

//     // GGX NDF
//     float cos2 = m.z * m.z;
//     float tan2 = (1.0f - cos2) / cos2;
//     float e = a2 + tan2;
//     float ndf = a2 / (PI_F32 * cos2 * cos2 * e * e);

//     float len2 = a2 * dot(i.xy, i.xy);
//     float t = sqrt(len2 + i.z * i.z);

//     if (i.z >= 0.0f)
//         return ndf / (2.0f * (i.z + t));  // only difference: i.z instead of k*i.z

//     return ndf * (t - i.z) / (2.0f * len2);
// }

// float3 sample_vndf_isotropic(float2 u, float3 wi, float alpha, float3 n)
// {
//     // decompose the floattor in parallel and perpendicular components
//     float3 wi_z = -n * dot(wi, n);
//     float3 wi_xy = wi + wi_z;

//     // warp to the hemisphere configuration
//     float3 wiStd = -normalize(alpha * wi_xy + wi_z);

//     // sample a spherical cap in (-wiStd.z, 1]
//     float wiStd_z = dot(wiStd, n);
//     float z = 1.0 - u.y * (1.0 + wiStd_z);
//     float sinTheta = sqrt(saturate(1.0f - z * z));
//     float phi = PI_F32 * 2.0f * u.x - PI_F32;
//     float x = sinTheta * cos(phi);
//     float y = sinTheta * sin(phi);
//     float3 cStd = float3(x, y, z);

//     // reflect sample to align with normal
//     float3 up = float3(0, 0, 1.000001);  // Used for the singularity
//     float3 wr = n + up;
//     float3 c = dot(wr, cStd) * wr / wr.z - cStd;

//     // compute halfway direction as standard normal
//     float3 wmStd = c + wiStd;
//     float3 wmStd_z = n * dot(n, wmStd);
//     float3 wmStd_xy = wmStd_z - wmStd;

//     // return final normal
//     return normalize(alpha * wmStd_xy + wmStd_z);
// }

// float pdf_vndf_isotropic(float3 wo, float3 wi, float alpha, float3 n)
// {
//     float alphaSquare = alpha * alpha;
//     float3 wm = normalize(wo + wi);
//     float zm = dot(wm, n);
//     float zi = dot(wi, n);
//     float nrm = rsqrt((zi * zi) * (1.0f - alphaSquare) + alphaSquare);
//     float sigmaStd = (zi * nrm) * 0.5f + 0.5f;
//     float sigmaI = sigmaStd / nrm;
//     float nrmN = (zm * zm) * (alphaSquare - 1.0f) + 1.0f;
//     return alphaSquare / (PI_F32 * 4.0f * nrmN * nrmN * sigmaI);
// }

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Quarter-rate shading */
    // const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
    const int2 pixel_id = (thread_id << 1); // + offset;

    /* Bounds check */
    if (any(pixel_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[pixel_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) {
        specular[thread_id] = 0.0f;
        return;
    }

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, pixel_id);

    /* Unpack visibility buffer data */
    const GeoData geo = unpack_geometry(unpack_visibility(facet_key), objects, ray);
    const float eta = geo.material.ior() / 1.0f;
    if (eta <= 1.0f) {
        specular[thread_id] = 0.0f;
        return; /* No reflections */
    }

    const float3 hit_point = ray.origin + ray.dir * geo.depth + geo.normal * 0.001f;

    const float2 noise = r2(blue_noise[thread_id % 512].rg, render_view.frame_index % 1024u);

    /* Transform vectors into shading space */
    const float3x3 basis = orthonormal_basis(geo.normal);
    const float3 wo = normalize(mul(basis, -ray.dir));

    /* Get the material roughness */
    const float roughness = geo.material.roughness();
    const float alpha = roughness * roughness;

    /* Importance sample specular lobe */
    const float alpha_2 = alpha * alpha;
    const float3 wh = sample_vndf_bounded(wo, alpha, alpha_2, noise);
    const float3 wi = normalize(reflect(-wo, wh));
    // const float3 wi = sample_cone(float3(-wo.x, -wo.y, wo.z), cos(0.1f), noise);
    // const float3 wh = normalize(wi + wo);

    /* Ignore directions which lie underneath the horizon */
    // if (wi.z <= 0.0f) wi = float3(-wo.x, -wo.y, wo.z);
    // if (wi.z <= 0.0f) {
    //     specular[thread_id] = 0.0f;
    //     return;
    // }
    const float3 sample_dir = normalize(mul(transpose(basis), wi));

    /* Evaluate the BSDF and PDF of the sample */
    const float pdf = 1.0f;  // pdf_vndf_isotropic(wo, wi, alpha, float3(0, 0, 1));
    const float3 bsdf = specular_brdf(wi.z, wo.z, wh.z, dot(wo, wh), geo.material) / pdf;
    // float3 wi;
    // const float3 bsdf = ImportanceSampleGgxD(wo, wi, alpha, eta, noise);

    /* Trace sample direction */
    const Hit hit = trace_bvh2<false>(tlas_nodes, object_indices, objects, custom_ray(hit_point, sample_dir), BIG_F32);

    float3 luminance;
    if (hit.t < BIG_F32) {
        /* Sample material emission */
        const Material mat = unpack_material(hit, objects);
        luminance = mat.albedo() * mat.emission();

        /* Sample surface cache for diffuse reflection */
        const cache::Slot slot = cache::fetch(macrofacet_cache, pack_visibility(hit));
        if (slot.key != cache::EMPTY_KEY) luminance += mat.albedo() * slot.read_diffuse();
    } else {
        const float2 uv = dir_to_latlong(sample_dir);
        luminance = bindless::texture_sampled<Texture2D<float4>>(scene_view.envmap_full_handle).SampleLevel(envmap_sampler, uv, 0).rgb;
    }
    luminance *= bsdf;

    specular[thread_id] = luminance;
}
