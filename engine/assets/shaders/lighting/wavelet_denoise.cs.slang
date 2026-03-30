import view;
import visibility;
import math;
import voxel;
import colorspace;
import svt64;
import bindless;

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

bool fetch_center(const int2 thread_id, out float3 c, out float3 n, out float3 p, out float r) {
    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[thread_id << 1]);
    c = lbuffer_in[thread_id];
    n = 0.0f; p = 0.0f; r = 0.0f;

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return false;
    const VisData vis = unpack_visibility(facet_key);
    const VoxelObject object = objects[vis.object_index];

    /* Unpack the geometry normal (1b sign | 2b axis) */
    float3 local_normal = 0.0f;
    local_normal[vis.face_index & 0b11u] = (vis.face_index >> 2) == 0u ? -1.0f : 1.0f;
    n = normalize(mul(object.local_to_world, float4(local_normal, 0.0f)).xyz);

    /* Unpack the voxel position index (1D -> 3D) */
    const uint3 voxel_index = expand_voxel_index(vis.voxel_index);

    /* Calculate the voxel face position in world-space */
    const float3 voxel_pos = (float3(voxel_index) + 0.5f + local_normal * 0.5f) * UNITS_PER_VOXEL;
    const float3 half_extent = float3(object.size_rcp_width.xyz) * 0.5f * UNITS_PER_VOXEL;
    p = mul(object.local_to_world, float4(voxel_pos - half_extent, 1.0f)).xyz;

    /* Find the voxel data pointer by traversing the SVT64 */
    const StructuredBuffer<Svt64Node> nodes = bindless::storage_buffer<StructuredBuffer<Svt64Node>>(object.blas_handle);
    const uint voxel_ptr = fetch_svt64(nodes, object.tree_depth, voxel_index);

    /* Fetch the material roughness */
    r = voxel_material(object, voxel_ptr).roughness();
    // r *= r;
    return r <= 0.05f ? false : true;
}

Sample fetch_sample(const int2 vpixel, const int2 lpixel) {
    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[vpixel]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return Sample(0.0f, 0.0f, 0.0f, 0.0f);

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
        lbuffer_in[lpixel],
        normal,
        world_voxel // ray.origin + ray.dir * depth
    );
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    const int2 pixel_id = thread_id << 1;

    /* Bounds check */
    if (any(pixel_id >= render_view.resolution)) return;

    /* Fetch the center data */
    float3 color, normal, point;
    float roughness;
    if (fetch_center(thread_id, color, normal, point, roughness) == false) {
        lbuffer_out[thread_id] = color;
        return;
    }

    float3 sum = 0.0f;
    float cum_weight = 0.0f;

    const float c_phi = 1.0f / ((float)step_size * 0.5f);
    // static const float N_PHI = 0.01f;
    static const float P_PHI = 0.5f;

    [ForceUnroll] for (uint i = 0u; i < 9u; ++i) {
        /* Sample neighbour */
        const int2 lpixel = thread_id + offsets[i] * step_size;
        const int2 vpixel = lpixel << 1;
        const Sample sample = fetch_sample(vpixel, lpixel);
        if (dot(sample.normal, normal) <= 0.01f) continue;

        /* Compute edge-stopping weights */
        // const float color_d = dot(color - sample.color, color - sample.color);
        // const float color_w = 1.0f;  // exp(-color_d / c_phi);
        const float pos_d = dot(point - sample.position, point - sample.position);
        const float pos_w = exp(-pos_d / (P_PHI * roughness));
        const float weight = pos_w * weights[i];
        // const float weight = weights[i];

        /* Update sum & cummulative weight */
        sum += sample.luminance * weight;
        cum_weight += weight;
    }

    lbuffer_out[thread_id] = sum / cum_weight;
}
