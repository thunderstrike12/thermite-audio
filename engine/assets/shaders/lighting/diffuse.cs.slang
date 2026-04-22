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

struct PickedLight {
    uint32_t i;    /* Selected light index. */
    float rcp_pdf; /* Reciprocal probability distribution function. */
};

inline float3 direct_illuminance(const float3 point, const float3 normal, const float3 view_dir, const Material mat, inout uint seed, const float2 noise) {
    float3 illuminance = mat.albedo() * mat.emission();

    /* Handle the sun light source */
    if (any(scene_view.sun_luminance > 0.0f)) {
        /* Generate and trace a sun light sample */
        const LightSample light_sample = sample_sun(point, scene_view, noise);
        const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, custom_ray(point, light_sample.dir), light_sample.tmax);

        if (t >= light_sample.tmax) {
            /* Calculate sun illuminance */
            const float ndotl = max(0.0f, dot(normal, light_sample.dir));
            illuminance += scene_view.sun_luminance * ndotl * light_sample.rcp_pdf;
        }
    }

    /* Index light grid cascades */
    PickedLight picked_light;
    const lgrid::CascadeIndex index = lgrid::index_cascades(point - grid_center);
    if (index.cascade == uint::maxValue) return illuminance;

    /* Compute cell index and base, and retrieve light count */
    const uint32_t cell_base = lgrid::cell_base(index.cascade, index.x, index.y, index.z);
    const uint32_t light_count = light_grid[cell_base];
    if (light_count == 0u) return illuminance;

    /* Pick light */
    const uint32_t slot = uint32_t(rand_float(seed) * light_count);
    const uint32_t light_index = light_grid[cell_base + 1u + slot];
    picked_light = PickedLight(light_index, (float)light_count);

    /* Fetch a light to sample */
    const Light light = lights[picked_light.i];

    /* Generate a sample for the light we picked */
    const LightSample light_sample = sample_light(point, light, noise);
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, custom_ray(point, light_sample.dir), light_sample.tmax);
    if (t < light_sample.tmax)
        return illuminance;

    /* Distance based attenuation */
    const float relative_dist = saturate(light_sample.tmax / light.attenuation_radius);
    const float inverse_dist = 1.0f - relative_dist * relative_dist;
    float attenuation = (inverse_dist * inverse_dist) / (1.0f + 4.0f * relative_dist);

    /* Spot based attenuation */
    if (light.type == LightType::SPOT_LIGHT) {
        const float3 light_dir = normalize(point - light.origin);
        const float cos_theta = dot(light_dir, light.direction);
        const float cos_inner = lerp(light.cos_beam_angle, 1.0f, light.spot_blend * 2.0f);
        const float spot = saturate((cos_theta - light.cos_beam_angle) / (cos_inner - light.cos_beam_angle));
        attenuation *= spot * spot;
    }

    /* Correct BRDF is 1.0f / PI however this cancels out with the PDF being 1.0f / PI, so it's just 1.0f */
    const float brdf = 1.0f;

    /* Calculate light illuminance */
    const float ndotl = max(0.0f, dot(normal, light_sample.dir));
    illuminance += light.luminance * ndotl * attenuation * brdf * picked_light.rcp_pdf * light_sample.rcp_pdf;
    return illuminance;
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
    const float3 diffuse = direct_illuminance(hit_point, geo.normal, -ray.dir, geo.material, seed, noise);

    lbuffer[thread_id] = scale * diffuse + 0.0025f;
}
