module voxel;

import math;
import view;
import svt64;
import material;
import bindless;

/* Represents a renderable voxel object. */
public struct VoxelObject {
    public float4x4 world_to_local;
    public float4x4 local_to_world;
    public float4x4 prev_local_to_world;
    public uint4 size_rcp_width; /* .xyz = voxel size ~ .w = reciprocal tree width */
    public uint tree_depth; /* Depth of this objects SVT64. */
    public uint blas_handle;
    public uint voxels_handle;
    public uint palette_handle;
    public uint object_flags; /* b0 = outlined */
};

/* Get the face_index of the local-space AABB, in case we instantly hit a voxel in the object. */
inline uint aabb_face_index(float3 entry_uvw) {
    const float3 entry_ndc = entry_uvw * 2.0f - 1.0f;
    const float2 entry_abs = abs(entry_ndc.xy);
    const float max_axis = max3(entry_abs.x, entry_abs.y, abs(entry_ndc.z));

    if (entry_abs.x == max_axis) return entry_ndc.x >= 0.0f ? 0b100u : 0b000u;
    else if (entry_abs.y == max_axis) return (entry_ndc.y >= 0.0f ? 0b100u : 0b000u) | 1u;
    return (entry_ndc.z >= 0.0f ? 0b100u : 0b000u) | 2u;
}

public [ForceInline] void trace_voxel_object<let DEBUG : bool>(Ray ray, VoxelObject object, uint index, inout Hit hit) {
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
    const float t_far = min(min(min(t_max.x, t_max.y), t_max.z), hit.t);
    if (t_near > t_far) return;

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
    if (DEBUG) hit.steps += local_hit.steps;
    if (local_hit.miss()) return;

    /* Calculate world-space hit point and distance */
    const float3 hit_uvw = (local_hit.point - 1.0f) / width_ratio;
    const float3 hit_ndc = hit_uvw * 2.0f - 1.0f;
    const float3 hit_point = mul(object.local_to_world, float4(hit_ndc * half_extent, 1.0f)).xyz;
    const float hit_dist = distance(world_origin, hit_point);
    if (hit_dist > hit.t) return;

    /* Overwrite the current hit with the closer one */
    hit.t = hit_dist;
    hit.voxel_index = local_hit.voxel_index;
    hit.object_index = index;
    if (local_hit.face_index == 3u && t_near > 0.0f) {
        /* Use the bounding box normals if we didn't get a normal from the SVT64 */
        hit.face_index = aabb_face_index(entry_uvw);
    } else {
        hit.face_index = local_hit.face_index;
    }
}

public [ForceInline] void occlusion_voxel_object(Ray ray, VoxelObject object, uint index, inout float t) {
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
    if (t_near > t_far) return;

    /* Calculate the ratio between the object size and its tree size */
    const float3 width_ratio = float3(object.size_rcp_width.xyz) * asfloat(object.size_rcp_width.w);

    /* Calculate the entry point for the tree traversal */
    const float3 entry_point = ray.origin + ray.dir * t_near;
    const float3 entry_uvw = (entry_point / half_extent) * 0.5f + 0.5f;
    ray.origin = (entry_uvw * width_ratio) + 1.0f; /* [1.0, 2.0) */

    /* Trace the SVT64 acceleration structure */
    const StructuredBuffer<Svt64Node> nodes = bindless::storage_buffer<StructuredBuffer<Svt64Node>>(object.blas_handle);
    const float3 local_hit = occlusion_svt64(nodes, ray);

    /* Check if we missed */
    if (local_hit.x == float::maxValue) return;

    /* Calculate world-space hit point and distance */
    const float3 hit_uvw = (local_hit - 1.0f) / width_ratio;
    const float3 hit_ndc = hit_uvw * 2.0f - 1.0f;
    const float3 hit_point = mul(object.local_to_world, float4(hit_ndc * half_extent, 1.0f)).xyz;
    const float hit_dist = distance(world_origin, hit_point);
    if (hit_dist > t) return;

    /* Overwrite the current hit with the closer one */
    t = hit_dist;
}

uint jenkins32(uint a) {
    a = (a + 0x7ed55d16u) + (a << 12);
    a = (a ^ 0xc761c23cu) ^ (a >> 19);
    a = (a + 0x165667b1u) + (a << 5);
    a = (a + 0xd3a2646cu) ^ (a << 9);
    a = (a + 0xfd7046c5u) + (a << 3);
    a = (a ^ 0xb55a4f09u) ^ (a >> 16);
    return a;
}

public inline uint voxel_hash(const uint voxel_index, const uint object_index, const uint face_index) {
    return jenkins32(voxel_index) ^ jenkins32(object_index | (face_index << 16));
}

public inline uint voxel_hash(const uint64_t packed) {
    const uint object_index = (uint)((packed >> 32) & 0xFFFFu);
    const uint voxel_index = (uint)(packed & 0xFFFFFFFFu);
    const uint face_index = (uint)(packed >> 48);
    return voxel_hash(voxel_index, object_index, face_index);
}
