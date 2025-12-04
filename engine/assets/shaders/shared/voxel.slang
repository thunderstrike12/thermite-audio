module voxel;

import math;
import view;

/* Represents a renderable voxel object. */
public struct VoxelObject {
    float4x4 world_to_local;
    float4x4 local_to_world;
    uint3 size;
};

public inline void intersect_voxel_object(Ray ray, VoxelObject object, out float hit_t) {
    /* Transform the ray into the object local-space */
    const float3 local_origin = mul(object.world_to_local, float4(ray.origin, 1.0f)).xyz;
    const float3 local_rcp_dir = 1.0f / mul(object.world_to_local, float4(ray.dir, 0.0f)).xyz;

    /* Intersect the object AABB in local-space */
    const float3 half_extent = float3(object.size) * UNITS_PER_VOXEL * 0.5f;
    const float3 t_to_min = (-half_extent - local_origin) * local_rcp_dir;
    const float3 t_to_max = (half_extent - local_origin) * local_rcp_dir;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    hit_t = t_near > t_far ? BIG_F32 : t_near;
}
