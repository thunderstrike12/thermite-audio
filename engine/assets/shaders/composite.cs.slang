import view;
import visibility;
import math;
import voxel;
import cache;
import colorspace;
import bindless;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Active scene view. */
ConstantBuffer<SceneView> scene_view;
/* Environment map sampler. */
SamplerState envmap_sampler;
/* Object buffers. */
StructuredBuffer<VoxelObject> objects;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Luminance buffer. */
Texture2D<float3> lbuffer;
/* Render target. */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[thread_id]);

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    float3 luminance = 0.0f;
    if (null_visibility(packed)) {
        /* Sample environment */
        if (scene_view.envmap_full_handle != 0u) {
            const float2 uv = dir_to_latlong(ray.dir);
            luminance = bindless::texture_sampled<Texture2D<float4>>(scene_view.envmap_full_handle).SampleLevel(envmap_sampler, uv, 0).rgb;
        } else {
            /* Default environment color */
            render_target[thread_id] = float4(0.247f, 0.247f, 0.247f, 1.0f);
            return;
        }
    } else {
        /* Sample geometry */
        const cache::Slot slot = cache::sample(macrofacet_cache, packed, render_view.frame_index);

        if (slot.key == cache::EMPTY_KEY) {
            /* This is bad! */
            luminance = float3(10.0f, 0.0f, 10.0f);
        } else {
            /* Unpack visibility buffer data */
            const VisData vis = unpack_visibility(packed);
            const GeoData geo = unpack_geometry(vis, objects, ray);

            luminance = slot.read_diffuse() * geo.material.albedo();
        }
    }

    /* Convert from ACEScg to r709 */
    luminance = cs::acescg_to_r709(luminance);

    /* Apply AGX tonemapping */
    luminance = cs::agx(luminance);
    luminance = cs::agx_look(luminance);
    luminance = cs::agx_eotf(luminance);

    /* Output device transform */
    luminance = cs::delinearize(luminance);

    render_target[thread_id] = float4(luminance, 1.0f);
}
