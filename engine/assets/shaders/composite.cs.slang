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
RWTexture2D<float3> lbuffer;

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
    if (null_visibility(packed) == false) {
        const cache::Slot slot = cache::fetch(macrofacet_cache, packed);

        if (slot.key == cache::EMPTY_KEY) {
            /* This is bad! */
            luminance = float3(0.0f, 0.0f, 0.0f);
        } else {
            /* Unpack visibility buffer data */
            const VisData vis = unpack_visibility(packed);
            const GeoData geo = unpack_geometry(vis, objects, ray);

            luminance = slot.read_diffuse() * geo.material.albedo() + slot.read_specular();
        }
    }

    lbuffer[thread_id] = luminance;
}
