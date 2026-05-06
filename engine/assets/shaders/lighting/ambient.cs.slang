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
import lights;
import material;
import lightgrid;
import bindless;

/* Active render & scene view. */
ConstantBuffer<RenderView> render_view;
ConstantBuffer<SceneView> scene_view;
Texture2D<float4> blue_noise;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint32_t> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Luminance buffer. */
RWTexture2D<float3> lbuffer;
SamplerState linear_sampler;

static const float GI_RANGE = 16.0f /* m */;

inline float3 ambient_occlusion(const float3 point, const float3 normal, const float2 noise) {
    /* Generate a sample for the light we picked */
    const float3 ambient_dir = sample_hemisphere_cosine(normal, noise);
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, custom_ray(point, ambient_dir), GI_RANGE);
    if (t < GI_RANGE) return 0.0f;

    /* In case there's no envmap, use a base gray color */
    if (scene_view.envmap_filtered_handle == 0u) 
        return float3(0.1f, 0.1f, 0.1f);

    /* Sample the filtered envmap */
    const float2 uv = dir_to_latlong(ambient_dir);
    const float3 env = bindless::texture_sampled<Texture2D<float4>>(scene_view.envmap_filtered_handle).SampleLevel(linear_sampler, uv, 0).rgb * 4.0f;
    return env;
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
    const float2 noise = r2(blue_noise[pixel_id % 512].rg, render_view.frame_index % 1024u);
    const float3 diffuse = ambient_occlusion(hit_point, geo.normal, noise);

    lbuffer[thread_id] += scale * diffuse;
}
