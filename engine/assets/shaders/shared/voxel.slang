module voxel;

import math;
import view;
import svt64;
import material;
import bindless;

/* Represents a renderable voxel object. */
public struct VoxelObject {
    float4x4 world_to_local;
    float4x4 local_to_world;
    uint size_x;
    uint size_y;
    uint size_z;
    uint blas_handle;
    uint voxels_handle;
    uint palette_handle;
};

public inline Hit intersect_voxel_object<let DEBUG : bool>(Ray ray, VoxelObject object) {
    /* Transform the ray into the local space of the object */
    const float3 ray_start = ray.origin;
    ray.origin = mul(object.world_to_local, float4(ray.origin, 1.0f)).xyz;
    ray.dir = mul(object.world_to_local, float4(ray.dir, 0.0f)).xyz;
    ray.rcp_dir = 1.0f / ray.dir;

    /* Intersect the object AABB in local-space */
    const float3 half_extent = float3(object.size_x, object.size_y, object.size_z) * UNITS_PER_VOXEL * 0.5f;
    const float3 t_to_min = (-half_extent - ray.origin) * ray.rcp_dir;
    const float3 t_to_max = (half_extent - ray.origin) * ray.rcp_dir;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    if (t_near > t_far) return Hit::miss();

#if 0
    Hit hit;
    hit.t = t_near;
    hit.steps = 1u;
    return hit;
#else
    /* Move to the point of entry */
    ray.origin = ray.origin + ray.dir * t_near;

    /* Move the origin into [1, 2) space for 64 tree traversal */
    ray.origin = ray.origin / half_extent * 0.5f + 1.5f;

    /* Fetch the SVT64 buffers */
    const StructuredBuffer<Svt64Node> nodes = bindless::storage_buffer<StructuredBuffer<Svt64Node>>(object.blas_handle);
    const StructuredBuffer<uint> voxels = bindless::storage_buffer<StructuredBuffer<uint>>(object.voxels_handle);
    const StructuredBuffer<Material> palette = bindless::storage_buffer<StructuredBuffer<Material>>(object.palette_handle);

    /* Trace the SVT64 */
    const Svt64Hit local_hit = trace_svt64<DEBUG>(nodes, voxels, palette, ray);

    Hit hit = Hit::miss(); /* Fill-in hit info */
    if (DEBUG) hit.steps = local_hit.steps;
    if (local_hit.miss()) return Hit::miss();

    /* Calculate world-space hit point and distance */
    const float3 hit_point = mul(object.local_to_world, float4((local_hit.point - 1.5f) * 2.0f * half_extent, 1.0f)).xyz;
    hit.t = distance(hit_point, ray_start);
    hit.albedo = local_hit.albedo;
    return hit;
#endif
}
