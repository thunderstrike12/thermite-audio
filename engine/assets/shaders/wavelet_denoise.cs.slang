import view;
import visibility;
import math;
import voxel;
import colorspace;

/* Active render view */
ConstantBuffer<RenderView> render_view;
/* Voxel objects buffer */
StructuredBuffer<VoxelObject> objects;
/* Visibility buffer */
Texture2D<uint2> vbuffer;
/* Luminance buffer (input) */
Texture2D<float3> lbuffer_in;
/* Luminance buffer (output) */
RWTexture2D<float3> lbuffer_out;

[[vk::push_constant]] uint step_size;

static const float weights[9] = {
    1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
    2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
    1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
};

static const int2 offsets[9] = {
    int2(-1, -1), int2(0, -1), int2(1, -1),
    int2(-1, 0), int2(0, 0), int2(1, 0),
    int2(-1, 1), int2(0, 1), int2(1, 1)
};

struct Sample {
    float3 luminance;
    float3 color;
    float3 normal;
    float3 position;
};

Sample fetch_sample(const int2 vpixel, const int2 lpixel) {
    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[vpixel]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return Sample();

    const VisData vis = unpack_visibility(facet_key);
    const VoxelObject object = objects[vis.object_index];

    /* Unpack the geometry normal (1b sign | 2b axis) */
    float3 local_normal = 0.0f;
    local_normal[vis.face_index & 0b11u] = (vis.face_index >> 2) == 0u ? -1.0f : 1.0f;
    const float3 normal = normalize(mul(object.local_to_world, float4(local_normal, 0.0f)).xyz);

    /* Unpack the voxel position index (1D -> 3D) */
    const uint3 voxel_index = expand_voxel_index(vis.voxel_index);

    /* Calculate the voxel face position in world-space */
    const float3 voxel_pos = (float3(voxel_index) + 0.5f + local_normal * 0.5f) * UNITS_PER_VOXEL;
    const float3 half_extent = float3(object.size_rcp_width.xyz) * 0.5f * UNITS_PER_VOXEL;
    const float3 world_voxel = mul(object.local_to_world, float4(voxel_pos - half_extent, 1.0f)).xyz;

    /* Find the depth of the geometry */
    // const Ray ray = pinhole_ray(render_view, vpixel);
    // const float depth = dot(world_voxel - ray.origin, normal) / dot(normal, ray.dir);

    return Sample(
        lbuffer_in[lpixel],
        log(1.0f + lbuffer_in[lpixel]),
        normal,
        world_voxel // ray.origin + ray.dir * depth
    );
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    int2 offset = 0;
    int2 shift = 0;
    if (render_view.shading_rate_di == 1u) {
        /* Half-rate */
        /*  [ XY ]  */
        /*  [ YX ]  */
        offset.x = (thread_id.y + render_view.frame_index) & 0b1;
        shift.x = 1;
    } else if (render_view.shading_rate_di == 2u) {
        /* Quarter-rate */
        /*  [ XY ]  */
        /*  [ ZW ]  */
        const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
        shift = 1;
    }
    const int2 pixel_id = (thread_id + offset) >> shift;

    /* Bounds check */
    if (any(pixel_id >= render_view.resolution)) return;

    const Sample center = fetch_sample(pixel_id, thread_id);

    float3 sum = 0.0f;
    float cum_weight = 0.0f;

    const float c_phi = 1.0f / ((float)step_size * 1.0f);
    static const float N_PHI = 0.5f;
    static const float P_PHI = 0.5f;

    [ForceUnroll] for (uint i = 0u; i < 9u; ++i) {
        /* Sample neighbour */
        const int2 lpixel = thread_id + offsets[i] * step_size;
        const int2 vpixel = (lpixel + offset) >> shift;
        const Sample sample = fetch_sample(vpixel, lpixel);

        /* Compute edge-stopping weights */
        const float color_d = dot(center.color - sample.color, center.color - sample.color);
        const float color_w = exp(-color_d / c_phi);
        const float normal_d = dot(center.normal - sample.normal, center.normal - sample.normal);
        const float normal_w = exp(-normal_d / N_PHI);
        const float pos_d = dot(center.position - sample.position, center.position - sample.position);
        const float pos_w = exp(-pos_d / P_PHI);
        const float weight = color_w * normal_w * pos_w * weights[i];

        /* Update sum & cummulative weight */
        sum += sample.luminance * weight;
        cum_weight += weight;
    }

    lbuffer_out[thread_id] = sum / cum_weight;
}
