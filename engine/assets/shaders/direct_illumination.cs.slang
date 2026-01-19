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

struct DiSettings {
    /* 0 = FullRate, 1 = HalfRate, 2 = QuarterRate */
    uint shading_rate;
};

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Settings buffer. */
ConstantBuffer<DiSettings> settings;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_illuminance_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    int2 pixel_id = thread_id;
    if (settings.shading_rate == 1u) {
        /* Half-rate */
        /*  [ XY ]  */
        /*  [ YX ]  */
        const int offset = (pixel_id.y + render_view.frame_index) & 0b1;
        pixel_id.x = (pixel_id.x << 1) + offset;
    } else if (settings.shading_rate == 2u) {
        /* Quarter-rate */
        /*  [ XY ]  */
        /*  [ ZW ]  */
        const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
        pixel_id = (pixel_id << 1) + offset;
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
    const float3 hit_point = ray.origin + ray.dir * geo.depth;

    /* Generate a seed for the current pixel */
    const uint seed = thread_id.x + thread_id.y * render_view.resolution.x + render_view.frame_index * render_view.resolution.x * render_view.resolution.y;

    /* Trace light occlusion */
    const float3 ray_dir = sample_hemisphere(geo.normal, r2(seed % 512));
    const Ray occlusion_ray = custom_ray(hit_point + geo.normal * 0.001f, ray_dir);
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, occlusion_ray, 1e30f);
    float3 illuminance = 0.0f;
    if (t >= 1e30f) {
        illuminance += float3(0.3f, 0.3f, 0.3f);
    }

    /* Insert the sample into the illuminance cache */
    cache::insert(macrofacet_illuminance_cache, facet_key, illuminance);
}
