import voxel;
import view;
import math;
import visibility;

/* Vertex shader output. */
struct VertexOut {
    float4 clip_pos : SV_POSITION;
};

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Voxel objects buffer. */
StructuredBuffer<VoxelObject> objects;

[shader("fragment")]
float main(VertexOut input) : SV_Depth {
    /* Generate a ray for our render view */
    const uint2 pixel = uint2(input.clip_pos.xy);

    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[pixel]);
    if (null_visibility(packed)) return 1.0f;

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, pixel);

    /* Unpack visibility buffer data */
    const VisData vis = unpack_visibility(packed);
    const VoxelObject object = objects[vis.object_index];

    /* Unpack the geometry normal (1b sign | 2b axis) */
    float3 local_normal = 0.0f;
    local_normal[vis.face_index & 0b11u] = (vis.face_index >> 2) == 0u ? -1.0f : 1.0f;
    const float3 world_normal = normalize(mul(object.local_to_world, float4(local_normal, 0.0f)).xyz);

    /* Unpack the voxel position index (1D -> 3D) */
    const uint3 voxel_index = expand_voxel_index(vis.voxel_index);

    /* Calculate the voxel face position in world-space */
    const float3 voxel_pos = (float3(voxel_index) + 0.5f + local_normal * 0.5f) * UNITS_PER_VOXEL;
    const float3 half_extent = float3(object.size_rcp_width.xyz) * 0.5f * UNITS_PER_VOXEL;
    const float3 world_voxel = mul(object.local_to_world, float4(voxel_pos - half_extent, 1.0f)).xyz;
    const float world_depth = dot(world_voxel - ray.origin, world_normal) / dot(world_normal, ray.dir);

    /* Write the depth buffer */
    float4 clip = mul(render_view.world_to_clip, float4(ray.origin + ray.dir * world_depth, 1.0f));
    return clip.z / clip.w;
}
