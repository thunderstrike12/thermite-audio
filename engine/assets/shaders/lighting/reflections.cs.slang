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
[ForceInline] float3 sample_vndf_bounded(float3 i, float alpha, float2 r01) {
    const float3 i_std = normalize(float3(i.xy * alpha, i.z));
    const float s = 1.0f + length(i.xy);
    const float a2 = alpha * alpha;
    const float s2 = s * s;
    const float k = (1.0f - a2) * s2 / (s2 + a2 * i.z * i.z);
    const float b = (i.z > 0.0f) ? k * i_std.z : i_std.z;
    const float phi = 2.0f * PI_F32 * r01.x;
    const float z = mad(1.0f - r01.y, 1.0f + b, -b);
    const float sin_theta = sqrt(saturate(1.0f - z * z));
    const float3 o_std = float3(sin_theta * cos(phi), sin_theta * sin(phi), z);
    const float3 m_std = i_std + o_std;
    return normalize(float3(m_std.xy * alpha, m_std.z));
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Quarter-rate shading */
    const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
    const int2 pixel_id = (thread_id << 1) + offset;

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

    float3 hit_point = ray.origin + ray.dir * geo.depth + geo.normal * 0.001f;

    const float2 noise = r2(blue_noise[thread_id % 512].rg, render_view.frame_index % 1024u);

    /* Transform vectors into shading space */
    const float3x3 basis = orthonormal_basis(geo.normal);
    const float3 wo = normalize(mul(basis, -ray.dir));

    /* Get the material roughness */
    const float roughness = geo.material.roughness();
    const float alpha = roughness * roughness;

    /* Importance sample specular lobe (clamped to remove odd noise) */
    const float3 wh = sample_vndf_bounded(wo, alpha, clamp(noise, 0.05f, 0.95f));
    float3 wi = reflect(-wo, wh);

    /* Ignore directions which lie underneath the horizon */
    if (wi.z <= 0.0f) wi = float3(-wo.x, -wo.y, wo.z);

    /* Evaluate the BSDF */
    const float3 bsdf = specular_brdf(wi.z, wo.z, wh.z, dot(wo, wh), geo.material);

    /* Trace sample direction */
    wi = normalize(mul(transpose(basis), wi));
    const Hit hit = trace_bvh2<false>(tlas_nodes, object_indices, objects, custom_ray(hit_point, wi), BIG_F32);

    float3 luminance;
    if (hit.t < BIG_F32) {
        /* Sample material emission */
        const Material mat = unpack_material(hit, objects);
        luminance = mat.albedo() * mat.emission();

        /* Sample surface cache for diffuse reflection */
        const cache::Slot slot = cache::fetch(macrofacet_cache, pack_visibility(hit));
        if (slot.key != cache::EMPTY_KEY) luminance += mat.albedo() * slot.read_diffuse();
    } else {
        const float2 uv = dir_to_latlong(wi);
        luminance = bindless::texture_sampled<Texture2D<float4>>(scene_view.envmap_filtered_handle).SampleLevel(envmap_sampler, uv, 0).rgb * PI_F32 * 2.0f;
    }
    specular[thread_id] = luminance * bsdf;
}
