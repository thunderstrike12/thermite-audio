/**
 * @name Macrofacet Global Illumination (GI) [1 thread per pixel]
 * This pass computes lighting per pixel and writes the results directly into the illuminance cache.
 */
import view;
import math;
import visibility;
import voxel;
import bvh2;
import cache;
import bindless;

/* Active render & scene view. */
ConstantBuffer<RenderView> render_view;
ConstantBuffer<SceneView> scene_view;
Texture2D<float4> blue_noise;
/* Environment map sampler. */
SamplerState envmap_sampler;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Macrofacet cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Luminance buffer. */
RWTexture2D<float3> lbuffer;

static const float GI_RANGE = 32.0f/* m */;

inline float3 ambient_occlusion(const float3 point, const float3 normal, const float2 noise) {
    /* Generate a sample for the light we picked */
    const float3 ambient_dir = sample_hemisphere_cosine(normal, noise);
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, custom_ray(point, ambient_dir), GI_RANGE);
    if (t < GI_RANGE) return 0.0f;

    /* In case there's no envmap, use a base gray color */
    if (scene_view.envmap_filtered_handle == 0u) return float3(0.2475f, 0.2475f, 0.2475f); 

    /* Sample the filtered envmap */
    const float2 uv = dir_to_latlong(ambient_dir);
    const float3 env = bindless::texture_sampled<Texture2D<float4>>(scene_view.envmap_filtered_handle).SampleLevel(envmap_sampler, uv, 0).rgb;
    return env;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    // int2 pixel_id = thread_id;
    // int2 resolution = render_view.resolution;
    // if (render_view.shading_rate_gi == 1u) {
    //     /* Half-rate */
    //     /*  [ XY ]  */
    //     /*  [ YX ]  */
    //     const int offset = (pixel_id.y + render_view.frame_index) & 0b1;
    //     pixel_id.x = (pixel_id.x << 1) + offset;
    //     resolution.x = resolution.x >> 1;
    // } else if (render_view.shading_rate_gi == 2u) {
    //     /* Quarter-rate */
    //     /*  [ XY ]  */
    //     /*  [ ZW ]  */
    //     const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
    //     pixel_id = (pixel_id << 1) + offset;
    //     resolution = resolution >> 1;
    // }

    // /* Bounds check */
    // if (any(pixel_id >= render_view.resolution)) return;

    // /* Load visibility buffer data */
    // const uint64_t facet_key = asuint64(vbuffer[pixel_id]);

    // /* Exit early if the visibility for this pixel is null */
    // if (null_visibility(facet_key)) {
    //     lbuffer[thread_id] = 0.0f;
    //     return;
    // }

    // /* Generate a ray for our render view */
    // const Ray ray = pinhole_ray(render_view, pixel_id);

    // /* Unpack visibility data */
    // const GeoData geo = unpack_geometry(unpack_visibility(facet_key), objects, ray);
    // const float3 hit_point = ray.origin + ray.dir * geo.depth + geo.normal * 0.001f;

    // /* Ray trace global illuminance (AO for now) */
    // const float2 noise = r2(blue_noise[pixel_id % 512].rg, render_view.frame_index % 1024u);
    // const float3 illuminance = ambient_occlusion(hit_point, geo.normal, noise);

    // /* Insert the sample into the illuminance cache */
    // /* TODO: This is very hacky (illuminance * 8.0f), we should properly handle this */
    // cache::insert(macrofacet_cache, facet_key, illuminance * 8.0f, 0.0f);
    // lbuffer[thread_id] = illuminance;
}
