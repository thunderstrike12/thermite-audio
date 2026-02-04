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

struct DiSettings {
    /* 0 = FullRate, 1 = HalfRate, 2 = QuarterRate */
    uint shading_rate;
};

/* Active render & scene view. */
ConstantBuffer<RenderView> render_view;
ConstantBuffer<SceneView> scene_view;
/* Settings buffer. */
ConstantBuffer<DiSettings> settings;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Lights buffer. */
StructuredBuffer<Light> lights;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_illuminance_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;

struct PickedLight {
    uint i; /* Selected light index. */
    float rcp_pdf; /* Reciprocal probability distribution function. */
};

/* Pick a light to sample for a given sample point using reservoir sampling. */
inline PickedLight pick_light(const float3 point, const float3 normal, inout uint seed) {
    /* Reservoir state */
    uint i = uint::maxValue;
    float w_sum = 0.0f, w_i = BIG_F32;

    for (uint l = 0u; l < scene_view.light_count; ++l) {
        /* Calculate light weight */
        const float w = evaluate_weight(point, normal, lights[l]);

        w_sum += w; /* Update the reservoir */
        if (rand_float(seed) * w_sum < w) {
            i = l;
            w_i = w;
        }
    }

    return PickedLight(i, w_sum / w_i);
}

inline float3 direct_illuminance(const float3 point, const float3 normal, inout uint seed) {
    if (scene_view.light_count == 0u) return 0.0f;

    /* Fetch a light to sample */
    const PickedLight picked_light = pick_light(point, normal, seed);
    if (picked_light.i == uint::maxValue) return 0.0f;
    const Light light = lights[picked_light.i];

    const LightSample light_sample = sample_light(point, light, rand_float2(seed));
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, custom_ray(point, light_sample.dir), light_sample.tmax);
    if (t < light_sample.tmax) return 0.0f;

    float attenuation = 1.0f;
    /* Distance based attenuation */
    if (light.type != LightType::SUN_LIGHT) {
        const float relative_dist = saturate(light_sample.tmax / light.attenuation_radius);
        const float inverse_dist = 1.0f - relative_dist * relative_dist;
        attenuation = (inverse_dist * inverse_dist) / (1.0f + 4.0f * relative_dist);
    }

    /* Spot based attenuation */
    if (light.type == LightType::SPOT_LIGHT) {
        const float cos_theta = dot(-light_sample.dir, light.direction);
        const float cos_inner = lerp(light.cos_beam_angle, 1.0f, light.spot_blend);
        const float spot = saturate((cos_theta - light.cos_beam_angle) / (cos_inner - light.cos_beam_angle));
        attenuation *= spot * spot;
    }

    return light.luminance * max(0.0f, dot(normal, light_sample.dir)) * attenuation * picked_light.rcp_pdf * light_sample.rcp_pdf;

    /* Calculate the direction and distance to the light */
    // const float3 light_pos = light.origin + sample_hemisphere_uniform(normalize(light.origin - point), rand_float2(seed)) * light.source_radius;
    // const float3 light_dir = normalize(light_pos - point);
    // const float ndotl = max(0.0f, dot(normal, light_dir));
    // if (ndotl == 0.0f) return 0.0f;
    // const float light_t = distance(light_pos, point);
    // if (light_t >= light.attenuation_radius) return 0.0f;

    /* Trace a ray towards the light to check for occlusions */
    // const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, custom_ray(point, light_dir), light_t);
    // if (t < light_t) return 0.0f;

    // const float relative_dist = saturate(light_t / light.attenuation_radius);
    // const float inverse_dist = 1.0f - relative_dist * relative_dist;
    // const float attenuation = (inverse_dist * inverse_dist) / (1.0f + 4.0f * relative_dist);

    /* Evaluate illuminance if there is no occlusion */
    // return light.luminance * ndotl * attenuation * light_sample.rcp_pdf;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    int2 pixel_id = thread_id;
    int2 resolution = render_view.resolution;
    if (settings.shading_rate == 1u) {
        /* Half-rate */
        /*  [ XY ]  */
        /*  [ YX ]  */
        const int offset = (pixel_id.y + render_view.frame_index) & 0b1;
        pixel_id.x = (pixel_id.x << 1) + offset;
        resolution.x = resolution.x >> 1;
    } else if (settings.shading_rate == 2u) {
        /* Quarter-rate */
        /*  [ XY ]  */
        /*  [ ZW ]  */
        const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
        pixel_id = (pixel_id << 1) + offset;
        resolution = resolution >> 1;
    }

    /* Bounds check */
    if (any(pixel_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[pixel_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return;

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, pixel_id);

    /* Unpack visibility data */
    const GeoData geo = unpack_geometry(unpack_visibility(facet_key), objects, ray);
    const float3 hit_point = ray.origin + ray.dir * geo.depth + geo.normal * 0.001f;

    /* Generate a seed for the current pixel */
    uint seed = thread_id.x + thread_id.y * resolution.x + (render_view.frame_index * (resolution.x * resolution.y));

    float3 illuminance = direct_illuminance(hit_point, geo.normal, seed);

    /* Trace light occlusion */
    // const float3 ray_dir = sample_hemisphere_cosine(geo.normal, r2(seed));
    // const Ray gi_ray = custom_ray(hit_point, ray_dir);
    // const Hit hit = trace_bvh2<false>(tlas_nodes, object_indices, objects, gi_ray, 32.0f);
    // if (hit.t < 32.0f) {
    //     // const uint64_t hit_key = pack_visibility(hit);
    //     // illuminance += cache::sample(macrofacet_illuminance_cache, hit_key, render_view.frame_index);
    // } else {
    //     illuminance += 0.05f;
    // }

    /* Insert the sample into the illuminance cache */
    cache::insert(macrofacet_illuminance_cache, facet_key, illuminance);
}
