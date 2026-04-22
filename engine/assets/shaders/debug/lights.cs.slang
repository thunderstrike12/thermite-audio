import lights;
import view;
import voxel;
import debug;
import math;
import visibility;
import colorspace;
import lightgrid;

/* Active render view. (Input)*/
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<VoxelObject> objects;
/* Visibility buffer. (Input) */
Texture2D<uint2> vbuffer;
/* Light grid buffer. (Input) */
StructuredBuffer<uint32_t> light_grid;
/* Render target. (Input/Output)*/
RWTexture2D<float4> render_target;

static const float MAX_OCCUPANCY = 8.0f;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[thread_id]);

    if (null_visibility(packed)) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
    } else {
        /* Generate a ray for our render view */
        const Ray ray = pinhole_ray(render_view, thread_id);

        /* Unpack visibility buffer data */
        const VisData vis = unpack_visibility(packed);
        const GeoData geo = unpack_geometry(vis, objects, ray);

        /* Fetch the occupancy at the hit point */
        const uint occupancy = lgrid::light_occupancy(light_grid, ray.dir * geo.depth);

        /* Calculate some fake shading for the view */
        const float fake_shading = clamp(dot(-ray.dir, geo.normal), 0.2f, 0.8f);
        render_target[thread_id] = float4(heat_color(float(occupancy) / MAX_OCCUPANCY) * fake_shading, 1.0f);
    }
}
