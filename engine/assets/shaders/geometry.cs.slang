import view;
import math;

/* Ray AABB intersection function. *(returns BIG_F32 if the ray misses the AABB)* */
inline void intersect_aabb(float3 ro, float3 rrd, float3 bmin, float3 bmax, out float hit_t) {
    const float3 t_to_min = (bmin - ro) * rrd;
    const float3 t_to_max = (bmax - ro) * rrd;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    hit_t = t_near > t_far ? BIG_F32 : t_near;
}

struct VoxelObject {
    float4x4 world_to_local;
    uint3 size;
};

/* Bindings */
ConstantBuffer<RenderView> render_view;
StructuredBuffer<VoxelObject> object_buffer;
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);
    // render_target[thread_id] = float4(ray.dir * 0.5 + 0.5, 1.0);

    // const float3 world_dir = mul(render_view.clip_to_view, float4(ndc, -1.0f, 0.0f)).xyz;
    // render_target[thread_id] = float4(world_dir * 0.5 + 0.5, 1.0);

    float hit_t;
    intersect_aabb(ray.origin, ray.rcp_dir, 0.0, 1.0, hit_t);
    if (hit_t == BIG_F32) {
        render_target[thread_id] = float4(0.0, 0.0, 0.0, 1.0);
    } else {
        render_target[thread_id] = float4(1.0 - (hit_t * 0.25).xxx, 1.0);
    }
}
