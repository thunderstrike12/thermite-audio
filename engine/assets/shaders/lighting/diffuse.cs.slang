/**
 * @name Macrofacet Direct Illumination (DI) [1 thread per pixel]
 * This pass computes lighting per pixel and writes the results directly into the illuminance cache.
 */
import view;
import math;
import visibility;
import voxel;
import bvh2;
import cache;
import lights;
import material;
import lightgrid;

/* Active render & scene view. */
ConstantBuffer<RenderView> render_view;
ConstantBuffer<SceneView> scene_view;
Texture2D<float4> blue_noise;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint32_t> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Lights buffer. */
StructuredBuffer<Light> lights;
/* Light grid buffer. */
StructuredBuffer<uint32_t> light_grid;
/* Macrofacet cache buffer. */
// RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Luminance buffer. */
RWTexture2D<float3> lbuffer;

[[vk::push_constant]]
float3 grid_center;

static const float SUN_CHANCE = 0.5f;

inline float3 sample_lights(const float3 p, const float3 n, inout uint seed, const float2 noise) {
    LightSample s; /* Sample a light source */
    const float sun_chance = any(scene_view.sun_luminance > 0.0f) ? SUN_CHANCE : 0.0f;
    const bool sun_sample = rand_float(seed) < sun_chance;
    if (sun_sample) {
        /* Sample sun light */
        s = sample_sun(p, scene_view, noise);
    } else {
        /* Sample any direct light */
        s = lgrid::sample_light(light_grid, lights, p, grid_center, seed, noise);
    }

    /* Make sure the PDF is valid */
    if (s.rcp_pdf == 0.0f) return 0.0f;

    /* Evaluate NdotL */
    const float ndotl = max(0.0f, dot(n, s.dir));
    if (ndotl <= 0.0f) return 0.0f;

    /* Trace a ray */
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, custom_ray(p, s.dir), s.tmax);
    if (t < s.tmax) return 0.0f;

    /* Calculate the luminance for the light */
    float3 luminance = 0.0f;
    if (sun_sample) {
        /* Sun */
        luminance = scene_view.sun_luminance * (1.0f / sun_chance);
    } else {
        /* Direct light */
        const Light light = lights[s.index];

        /* Distance based attenuation */
        const float relative_dist = saturate(s.tmax / light.attenuation_radius);
        const float inverse_dist = 1.0f - relative_dist * relative_dist;
        float attenuation = (inverse_dist * inverse_dist) / (1.0f + 4.0f * relative_dist);

        /* Spot based attenuation */
        if (light.type == LightType::SPOT_LIGHT) {
            const float3 light_dir = normalize(p - light.origin);
            const float cos_theta = dot(light_dir, light.direction);
            const float cos_inner = lerp(light.cos_beam_angle, 1.0f, light.spot_blend * 2.0f);
            const float spot = saturate((cos_theta - light.cos_beam_angle) / (cos_inner - light.cos_beam_angle));
            attenuation *= spot * spot;
        }
        luminance = attenuation * light.luminance * (1.0f / (1.0f - sun_chance));
    }

    /* Correct BRDF is 1.0f / PI however this cancels out with the PDF being 1.0f / PI, so it's just 1.0f */
    const float brdf = 1.0f;

    /* Calculate light illuminance */
    return luminance * ndotl * brdf * s.rcp_pdf;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    const int2 pixel_id = rate_to_full(thread_id, render_view.diff_shading_rate);
    if (any(pixel_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[pixel_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) {
        lbuffer[thread_id] = 0.0f;
        return;
    }

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, pixel_id);

    /* Unpack visibility data */
    const GeoData geo = unpack_geometry(unpack_visibility(facet_key), objects, ray);

    /* Calculate diffuse scale based on metallicness */
    const float scale = saturate(1.0f - geo.material.metallic());
    if (scale == 0.0f) {
        /* Exit early if there's no diffuse contribution */
        lbuffer[thread_id] = 0.0f;
        return;
    }

    const float3 hit_point = ray.origin + ray.dir * geo.depth + geo.normal * 0.001f;

    /* Generate a seed for the current pixel */
    uint seed = pixel_id.x + pixel_id.y * render_view.resolution.x + (render_view.frame_index * (render_view.resolution.x * render_view.resolution.y));

    const float2 noise = r2(blue_noise[pixel_id % 512].rg, render_view.frame_index % 1024u);
    const float3 emission = geo.material.albedo() * geo.material.emission();
    const float3 diffuse = emission + sample_lights(hit_point, geo.normal, seed, noise);

    lbuffer[thread_id] = scale * diffuse;
}
