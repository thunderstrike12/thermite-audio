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
    uint4 size_rcp_width; /* .xyz = voxel size ~ .w = reciprocal tree width */
    uint blas_handle;
    public uint voxels_handle;
    public uint palette_handle;
};

public inline Hit intersect_voxel_object<let DEBUG : bool>(Ray ray, VoxelObject object, float t) {
    /* Transform the ray into the local space of the object */
    const float3 world_origin = ray.origin;
    ray.origin = mul(object.world_to_local, float4(ray.origin, 1.0f)).xyz;
    ray.dir = mul(object.world_to_local, float4(ray.dir, 0.0f)).xyz;
    ray.rcp_dir = 1.0f / ray.dir;

    /* Intersect the object AABB in local-space */
    const float3 half_extent = float3(object.size_rcp_width.xyz) * UNITS_PER_VOXEL * 0.5f;
    const float3 t_to_min = (-half_extent - ray.origin) * ray.rcp_dir;
    const float3 t_to_max = (half_extent - ray.origin) * ray.rcp_dir;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(min(t_max.x, t_max.y), t_max.z), t);
    if (t_near > t_far) return Hit::miss();

    /* Calculate the ratio between the object size and its tree size */
    const float3 width_ratio = float3(object.size_rcp_width.xyz) * asfloat(object.size_rcp_width.w);

    /* Calculate the entry point for the tree traversal */
    const float3 entry_point = ray.origin + ray.dir * t_near;
    const float3 entry_uvw = (entry_point / half_extent) * 0.5f + 0.5f;
    ray.origin = (entry_uvw * width_ratio) + 1.0f; /* [1.0, 2.0) */

    /* Trace the SVT64 acceleration structure */
    const StructuredBuffer<Svt64Node> nodes = bindless::storage_buffer<StructuredBuffer<Svt64Node>>(object.blas_handle);
    const Svt64Hit local_hit = trace_svt64<DEBUG>(nodes, ray);

    /* Check if we missed */
    Hit hit = Hit::miss();
    if (DEBUG) hit.steps = local_hit.steps;
    if (local_hit.miss()) return hit;
    hit.voxel_index = local_hit.voxel_index;

    /* Calculate world-space hit point and distance */
    const float3 hit_uvw = (local_hit.point - 1.0f) / width_ratio;
    const float3 hit_ndc = hit_uvw * 2.0f - 1.0f;
    const float3 hit_point = mul(object.local_to_world, float4(hit_ndc * half_extent, 1.0f)).xyz;
    hit.t = distance(world_origin, hit_point);
    return hit;
}
