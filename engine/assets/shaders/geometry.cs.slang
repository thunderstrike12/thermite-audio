import view;
import math;
import voxel;
import bvh2;

/* Ray AABB intersection function. *(returns BIG_F32 if the ray misses the AABB)* */
inline void intersect_aabb(float3 ro, float3 rrd, float3 bmin, float3 bmax, out float hit_t) {
    const float3 t_to_min = (bmin - ro) * rrd;
    const float3 t_to_max = (bmax - ro) * rrd;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    hit_t = t_near > t_far ? BIG_F32 : t_near;
}

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<float4> bvh_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> object_data;
/* Render target. */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Trace the scene BVH */
    const float hit_t = trace_bvh2(bvh_nodes, object_indices, object_data, ray) * 0.01;
    render_target[thread_id] = float4(hit_t, hit_t, hit_t, 1.0f);

    // if (hit_t < BIG_F32) {
    //     render_target[thread_id] = float4(1.0f - (hit_t * 0.25f).xxx, 1.0f);
    // } else {
    //     render_target[thread_id] = float4(0.05f, 0.05f, 0.05f, 1.0f);
    // }
}
